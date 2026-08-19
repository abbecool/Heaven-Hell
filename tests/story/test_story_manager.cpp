#include "TestSupport.hpp"
#include "story/StoryManager.hpp"

#include <array>
#include <string>

namespace {

using TestSupport::require;

std::string sourcePath(const std::string& relativePath)
{
#ifdef HEAVENHELL_SOURCE_DIR
    return std::string(HEAVENHELL_SOURCE_DIR) + "/" + relativePath;
#else
    return relativePath;
#endif
}

StoryManager makeStoryManager()
{
    StoryManager story(
        nullptr,
        sourcePath("config_files/story.json"),
        sourcePath("config_files/quests.json"));
    story.loadDialogs(sourcePath("config_files/dialogs.json"));
    return story;
}

void completeChapterOne(StoryManager& story)
{
    story.onEvent(Event{EventType::EntityDrained, "chicken"});
    story.onEvent(Event{EventType::EntityKilled, "knight"});
    story.onEvent(Event{EventType::EntityPossessed, "wizard"});
}

void testLoadsNewQuestFormat()
{
    StoryManager story = makeStoryManager();

    const Quest* consumeChicken = story.getQuest("chapter1.consume_chicken");
    require(consumeChicken != nullptr, "main story quest was not loaded");
    require(consumeChicken->title == "Regain Strength", "quest title was not parsed");
    require(consumeChicken->state == QuestState::Active, "initial quest state was not parsed");
    require(consumeChicken->steps.size() == 1, "quest step was not parsed");
    require(
        consumeChicken->steps[0].trigger.eventType == EventType::EntityDrained,
        "quest trigger event type was not parsed");

    const Quest* wizardQuest = story.getQuest("faction.wizard.start");
    require(wizardQuest != nullptr, "faction quest was not loaded");
    require(wizardQuest->state == QuestState::Locked, "faction quest should start locked");
}

void testTriggerMatchesMultipleSubjects()
{
    StoryManager story = makeStoryManager();

    story.onEvent(Event{EventType::EntityDrained, "chicken"});
    story.onEvent(Event{EventType::EntityKilled, "knight"});
    story.onEvent(Event{EventType::EntityPossessed, "elf"});

    const Quest* chooseHost = story.getQuest("chapter1.choose_host");
    require(chooseHost != nullptr, "choose-host quest was not loaded");
    require(chooseHost->state == QuestState::Completed, "multi-subject possession trigger did not match");
}

void testCompletesChapterOneInOrder()
{
    StoryManager story = makeStoryManager();

    story.onEvent(Event{EventType::EntityKilled, "knight"});
    const Quest* killKnight = story.getQuest("chapter1.kill_knight");
    require(killKnight != nullptr, "kill-knight quest was not loaded");
    require(killKnight->state == QuestState::Locked, "locked quest advanced from an early event");

    story.onEvent(Event{EventType::EntityDrained, "chicken"});
    require(killKnight->state == QuestState::Active, "chicken drain did not unlock knight quest");

    story.onEvent(Event{EventType::EntityKilled, "knight"});
    const Quest* chooseHost = story.getQuest("chapter1.choose_host");
    require(chooseHost != nullptr, "choose-host quest was not loaded");
    require(chooseHost->state == QuestState::Active, "knight kill did not unlock host choice");

    story.onEvent(Event{EventType::EntityPossessed, "wizard"});
    require(chooseHost->state == QuestState::Completed, "host choice did not complete");
}

void testUnlocksAllFactionTracks()
{
    StoryManager story = makeStoryManager();
    completeChapterOne(story);

    require(
        story.getQuest("faction.wizard.start")->state == QuestState::Active,
        "wizard faction track did not activate");
    require(
        story.getQuest("faction.elf.start")->state == QuestState::Active,
        "elf faction track did not activate");
    require(
        story.getQuest("faction.dwarf.start")->state == QuestState::Active,
        "dwarf faction track did not activate");
    require(
        story.getQuest("faction.knight.start")->state == QuestState::Active,
        "knight faction track did not activate");
}

void testIgnoresUnrelatedEvents()
{
    StoryManager story = makeStoryManager();

    story.onEvent(Event{EventType::EntityDrained, "goblin"});
    const Quest* consumeChicken = story.getQuest("chapter1.consume_chicken");
    require(consumeChicken != nullptr, "consume-chicken quest was not loaded");
    require(consumeChicken->state == QuestState::Active, "unrelated drain completed chicken quest");
    require(consumeChicken->currentStep == 0, "unrelated drain advanced current step");
}

void testSavesAndRestoresProgression()
{
    StoryManager story = makeStoryManager();
    completeChapterOne(story);
    story.onEvent(Event{EventType::DialogueFinished, "wizard"});

    const json saved = story.progressionJson();

    StoryManager restored = makeStoryManager();
    restored.loadProgression(saved);

    require(
        restored.getQuest("chapter1.choose_host")->state == QuestState::Completed,
        "completed main quest was not restored");
    require(
        restored.getQuest("faction.wizard.start")->state == QuestState::Completed,
        "completed faction quest was not restored");
    require(
        restored.getQuest("faction.elf.start")->state == QuestState::Active,
        "parallel faction state was not restored");
    require(saved.at("currentHost").get<std::string>() == "wizard", "current host was not saved");
}

void testDialogLookup()
{
    StoryManager story = makeStoryManager();

    const std::string& wizardOpening = story.getDialog("wizard");
    require(
        wizardOpening.find("little creature") != std::string::npos,
        "main quest dialog was not found by quest id");

    completeChapterOne(story);
    const std::string& wizardFaction = story.getDialog("wizard");
    require(
        wizardFaction.find("Magic opens") != std::string::npos,
        "faction dialog was not preferred for active NPC step");

    const std::string& chickenDefault = story.getDialog("chicken");
    require(chickenDefault == "...", "default dialog fallback was not used");
}

} // namespace

int main(int argc, char* argv[])
{
    static constexpr std::array tests{
        TestSupport::TestCase{"load_new_format", testLoadsNewQuestFormat},
        TestSupport::TestCase{"multi_subject_trigger", testTriggerMatchesMultipleSubjects},
        TestSupport::TestCase{"chapter_one_order", testCompletesChapterOneInOrder},
        TestSupport::TestCase{"unlock_faction_tracks", testUnlocksAllFactionTracks},
        TestSupport::TestCase{"ignore_unrelated_events", testIgnoresUnrelatedEvents},
        TestSupport::TestCase{"save_restore_progression", testSavesAndRestoresProgression},
        TestSupport::TestCase{"dialog_lookup", testDialogLookup}
    };

    return TestSupport::runNamedTest(argc, argv, tests);
}
