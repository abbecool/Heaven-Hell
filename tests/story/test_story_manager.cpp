#include "TestSupport.hpp"
#include "story/StoryManager.hpp"

#include <array>
#include <filesystem>
#include <fstream>
#include <string>

namespace {

using TestSupport::require;

const std::string StoryPath = std::string(HEAVENHELL_SOURCE_DIR) + "/config_files/story1.json";

Event event(EventType type, const std::string& subject)
{
    return Event{type, subject};
}

void advanceToFirstFace(StoryManager& story)
{
    story.onEvent(event(EventType::EntityDrained, "chicken"));
    require(story.isQuestActive("opening.leave_battlefield"), "draining chicken did not activate battlefield quest");
    story.onEvent(event(EventType::EntityKilled, "knight"));
    require(story.isQuestActive("world.choose_first_face"), "battlefield quest did not activate first-face quest");
}

void testEarlyPossessionIsCreditedOnce()
{
    StoryManager story(StoryPath);
    story.onEvent(event(EventType::EntityPossessed, "knight"));
    require(story.isQuestActive("opening.regain_strength"), "early possession advanced the opening quest");

    story.onEvent(event(EventType::EntityDrained, "chicken"));
    require(story.getQuestState("opening.leave_battlefield") == QuestState::Completed,
        "historic knight possession did not complete the newly active battlefield quest");
    require(story.isQuestActive("world.choose_first_face"),
        "historic possession cascaded past the intended first-face choice");

    story.onEvent(event(EventType::EntityPossessed, "dwarf"));
    require(story.isQuestActive("faction.dwarf.return_home"), "dwarf possession did not select dwarf route");
    require(story.getQuestState("faction.knight.return_home") == QuestState::Locked,
        "unchosen faction route was activated");

    story.onEvent(event(EventType::EnteredArea, "area.elf.home"));
    require(story.isQuestActive("faction.dwarf.return_home"), "wrong faction home completed selected route");
    story.onEvent(event(EventType::EnteredArea, "area.dwarf.home"));
    require(story.isQuestActive("main.kill_king"), "correct faction home did not activate final quest");
    story.onEvent(event(EventType::EntityKilled, "golem"));
    require(story.isStoryFinished(), "placeholder king death did not finish story");
}

void testFactionBranchesStayExclusive()
{
    const std::array<std::string, 4> factions = {"dwarf", "knight", "elf", "wizard"};
    for (const std::string& faction : factions) {
        StoryManager story(StoryPath);
        advanceToFirstFace(story);
        story.onEvent(event(EventType::EntityPossessed, faction));

        const std::string selectedQuest = "faction." + faction + ".return_home";
        require(story.isQuestActive(selectedQuest), "selected faction route was not activated");
        for (const std::string& otherFaction : factions) {
            if (otherFaction != faction) {
                require(story.getQuestState("faction." + otherFaction + ".return_home") == QuestState::Locked,
                    "unselected faction route was not kept locked");
            }
        }
    }
}

void testInvalidStoryIsRejected()
{
    const std::filesystem::path invalidPath = std::filesystem::temp_directory_path() / "heavenhell_invalid_story1.json";
    {
        std::ofstream invalidFile(invalidPath);
        invalidFile << "{}";
    }

    bool threw = false;
    try {
        StoryManager story(invalidPath.string());
    } catch (const std::runtime_error&) {
        threw = true;
    }
    std::filesystem::remove(invalidPath);
    require(threw, "invalid Story1 document was accepted");
}

constexpr std::array Tests = {
    TestSupport::TestCase{"early_possession_is_credited_once", testEarlyPossessionIsCreditedOnce},
    TestSupport::TestCase{"faction_branches_stay_exclusive", testFactionBranchesStayExclusive},
    TestSupport::TestCase{"invalid_story_is_rejected", testInvalidStoryIsRejected},
};

} // namespace

int main(int argc, char* argv[])
{
    return TestSupport::runNamedTest(argc, argv, Tests);
}
