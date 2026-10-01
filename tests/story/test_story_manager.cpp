#include "TestSupport.hpp"
#include "story/StoryManager.hpp"
#include "story/GameSave.hpp"

#include <array>
#include <filesystem>
#include <fstream>
#include <string>
#ifdef _WIN32
#include <windows.h>
#endif

namespace
{

using TestSupport::require;

const std::string StoryPath =
    std::string(HEAVENHELL_SOURCE_DIR) + "/config_files/story1.json";

Event event(EventType type, const std::string& subject)
{
    return Event{type, subject};
}

void advanceToFirstFace(StoryManager& story)
{
    story.onEvent(event(EventType::EntityDrained, "chicken"));
    require(story.isQuestActive("opening.leave_battlefield"),
            "draining chicken did not activate battlefield quest");
    story.onEvent(event(EventType::EntityKilled, "knight"));
    require(story.isQuestActive("world.choose_first_face"),
            "battlefield quest did not activate first-face quest");
}

void testEarlyPossessionIsCreditedOnce()
{
    StoryManager story(StoryPath);
    story.onEvent(event(EventType::EntityPossessed, "knight"));
    require(story.isQuestActive("opening.regain_strength"),
            "early possession advanced the opening quest");

    story.onEvent(event(EventType::EntityDrained, "chicken"));
    require(story.getQuestState("opening.leave_battlefield") ==
                QuestState::Completed,
            "historic knight possession did not complete the newly active "
            "battlefield quest");
    require(story.isQuestActive("world.choose_first_face"),
            "historic possession cascaded past the intended first-face choice");

    story.onEvent(event(EventType::EntityPossessed, "dwarf"));
    require(story.isQuestActive("faction.dwarf.return_home"),
            "dwarf possession did not select dwarf route");
    require(story.getQuestState("faction.knight.return_home") ==
                QuestState::Locked,
            "unchosen faction route was activated");

    story.onEvent(event(EventType::EnteredArea, "area.elf.home"));
    require(story.isQuestActive("faction.dwarf.return_home"),
            "wrong faction home completed selected route");
    story.onEvent(event(EventType::EnteredArea, "area.dwarf.home"));
    require(story.isQuestActive("main.kill_king"),
            "correct faction home did not activate final quest");
    story.onEvent(event(EventType::EntityKilled, "golem"));
    require(story.isStoryFinished(),
            "placeholder king death did not finish story");
}

void testFactionBranchesStayExclusive()
{
    const std::array<std::string, 4> factions = {"dwarf", "knight", "elf",
                                                 "wizard"};
    for (const std::string& faction : factions)
    {
        StoryManager story(StoryPath);
        advanceToFirstFace(story);
        story.onEvent(event(EventType::EntityPossessed, faction));

        const std::string selectedQuest = "faction." + faction + ".return_home";
        require(story.isQuestActive(selectedQuest),
                "selected faction route was not activated");
        for (const std::string& otherFaction : factions)
        {
            if (otherFaction != faction)
            {
                require(story.getQuestState("faction." + otherFaction +
                                            ".return_home") ==
                            QuestState::Locked,
                        "unselected faction route was not kept locked");
            }
        }
    }
}

void testInvalidStoryIsRejected()
{
    const std::filesystem::path invalidPath =
        std::filesystem::temp_directory_path() /
        "heavenhell_invalid_story1.json";
    {
        std::ofstream invalidFile(invalidPath);
        invalidFile << "{}";
    }

    bool threw = false;
    try
    {
        StoryManager story(invalidPath.string());
    }
    catch (const std::runtime_error&)
    {
        threw = true;
    }
    std::filesystem::remove(invalidPath);
    require(threw, "invalid Story1 document was accepted");
}

void testSaveAndRestoreProgress()
{
    StoryManager original(StoryPath);
    original.onEvent(event(EventType::EntityPossessed, "knight"));
    for (int i = 0; i < 200; ++i)
    {
        original.onEvent(event(EventType::EnteredArea, "area.dwarf.home"));
        original.onEvent(event(EventType::EntityPossessed, "knight"));
    }
    const auto saved = original.saveState();
    require(saved.at("prior_actions").size() == 1,
            "area contacts should not be stored for future quests");

    StoryManager resumed(StoryPath);
    resumed.loadState(saved);
    resumed.onEvent(event(EventType::EntityDrained, "chicken"));
    require(resumed.isQuestActive("world.choose_first_face"),
            "restored prior possession was not credited exactly once");
    resumed.onEvent(event(EventType::EntityPossessed, "dwarf"));
    require(resumed.isQuestActive("faction.dwarf.return_home"),
            "restored story did not allow the next choice");
    require(!resumed.isQuestActive("main.kill_king"),
            "earlier area contact incorrectly completed the new quest");

    StoryManager finalResume(StoryPath);
    finalResume.loadState(resumed.saveState());
    finalResume.onEvent(event(EventType::EnteredArea, "area.dwarf.home"));
    require(finalResume.isQuestActive("main.kill_king"),
            "saved quest step was not restored");
}

void testRejectChangedQuestDefinitions()
{
    StoryManager story(StoryPath);
    auto state = story.saveState();
    state["quests"][0]["steps"][0] = "different_step";
    bool rejected = false;
    try { story.loadState(state); }
    catch (const std::exception&) { rejected = true; }
    require(rejected, "save for different quest definitions was accepted");
    require(story.isQuestActive("opening.regain_strength"),
            "failed restoration changed live quest state");
}

void testSaveFileRoundTripAndFailure()
{
    const auto path = std::filesystem::temp_directory_path() /
                      "heavenhell_story_save_test.json";
    const GameSave save(path);
    const nlohmann::json snapshot = {{"version", 1}, {"player", nlohmann::json::object()},
                                     {"story", nlohmann::json::object()},
                                     {"world", nlohmann::json::object()}};
    save.write(snapshot);
    require(save.load() == snapshot, "saved game did not round-trip");
    bool rejected = false;
    try { save.write({{"version", 2}}); }
    catch (const std::exception&) { rejected = true; }
    require(rejected && save.load() == snapshot,
            "failed save replaced a valid prior save");
    std::filesystem::remove(path);
}

#ifdef _WIN32
void testDefaultSavePathSupportsUnicodeLocalAppData()
{
    constexpr wchar_t LocalAppData[] = L"LOCALAPPDATA";
    const DWORD previousSize = GetEnvironmentVariableW(LocalAppData, nullptr, 0);
    std::wstring previousValue;
    if (previousSize > 0)
    {
        previousValue.resize(previousSize);
        const DWORD copied = GetEnvironmentVariableW(
            LocalAppData, previousValue.data(), previousSize);
        require(copied > 0 && copied < previousSize,
                "could not capture LOCALAPPDATA for the test");
        previousValue.resize(copied);
    }

    const std::wstring unicodeRoot =
        L"C:\\Users\\Abbe\\AppData\\Local\\HeavenHell-ä";
    require(SetEnvironmentVariableW(LocalAppData, unicodeRoot.c_str()) != 0,
            "could not set Unicode LOCALAPPDATA for the test");

    std::filesystem::path actualPath;
    try
    {
        actualPath = GameSave::defaultPath();
    }
    catch (...)
    {
        if (previousSize > 0)
            SetEnvironmentVariableW(LocalAppData, previousValue.c_str());
        else
            SetEnvironmentVariableW(LocalAppData, nullptr);
        throw;
    }

    if (previousSize > 0)
        SetEnvironmentVariableW(LocalAppData, previousValue.c_str());
    else
        SetEnvironmentVariableW(LocalAppData, nullptr);

    const std::filesystem::path expectedPath =
        std::filesystem::path(unicodeRoot) / L"HeavenHell" / L"save.json";
    require(actualPath == expectedPath,
            "default save path did not preserve Unicode LOCALAPPDATA");
}
#endif

void testEventBusRoutesByTypeAndSubject()
{
    EventBus bus;
    int calls = 0;
    bus.subscribe(event(EventType::EntityKilled, "knight"),
                  [&calls](const Event&) { ++calls; });
    bus.emit(event(EventType::EntityPossessed, "knight"));
    bus.emit(event(EventType::EntityKilled, "dwarf"));
    require(calls == 0, "event bus delivered an unrelated event");
    bus.emit(event(EventType::EntityKilled, "knight"));
    require(calls == 1, "event bus lost matching event");
}

constexpr std::array Tests = {
    TestSupport::TestCase{"early_possession_is_credited_once",
                          testEarlyPossessionIsCreditedOnce},
    TestSupport::TestCase{"faction_branches_stay_exclusive",
                          testFactionBranchesStayExclusive},
    TestSupport::TestCase{"invalid_story_is_rejected",
                          testInvalidStoryIsRejected},
    TestSupport::TestCase{"save_and_restore_progress", testSaveAndRestoreProgress},
    TestSupport::TestCase{"reject_changed_quest_definitions", testRejectChangedQuestDefinitions},
    TestSupport::TestCase{"save_file_round_trip", testSaveFileRoundTripAndFailure},
#ifdef _WIN32
    TestSupport::TestCase{"default_save_path_supports_unicode_local_app_data",
                          testDefaultSavePathSupportsUnicodeLocalAppData},
#endif
    TestSupport::TestCase{"event_bus_routes_by_type_and_subject",
                          testEventBusRoutesByTypeAndSubject},
};

} // namespace

int main(int argc, char* argv[])
{
    return TestSupport::runNamedTest(argc, argv, Tests);
}
