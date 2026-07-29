#include "CognitiveAgent.h"
#include "MockLLMClient.h"
#include "MemorySystem.h"
#include "GoalManager.h"
#include "PerceptionSystem.h"
#include "AgentToolCalling.h"
#include "AgentCommunication.h"
#include <iostream>
#include <string>
#include <vector>

using namespace OpenMind;

static int testsPassed = 0;
static int testsFailed = 0;

void check(bool cond, const std::string& name) {
    if (cond) { testsPassed++; printf("  PASS: %s\n", name.c_str()); }
    else { testsFailed++; printf("  FAIL: %s\n", name.c_str()); }
}

void testMemorySystem() {
    printf("\n=== Memory System ===\n");

    CognitiveConfig cfg;
    cfg.maxShortTermMemory = 5;
    cfg.importanceThreshold = 0.3f;
    MemorySystem mem(cfg);

    mem.store("Saw stone blocks", MemoryType::OBSERVATION, 0.5f, 1);
    mem.store("Placed wood block", MemoryType::ACTION, 0.6f, 2);
    mem.store("Heard wolf nearby", MemoryType::OBSERVATION, 0.8f, 3);
    mem.store("Walked north", MemoryType::ACTION, 0.2f, 4);

    check(mem.shortTermCount() == 4, "4 memories stored");

    mem.store("Found food", MemoryType::DISCOVERY, 0.9f, 5);
    check(mem.shortTermCount() == 5, "At capacity");

    mem.store("Extra memory", MemoryType::ACTION, 0.1f, 6);
    check(mem.shortTermCount() == 5, "Evicted low-importance memory");

    auto recent = mem.retrieveRecent(3);
    check(recent.size() == 3, "Retrieve recent 3");

    std::string ctx = mem.buildContextString(3);
    check(!ctx.empty(), "Context string not empty");
    check(ctx.find("ACT") != std::string::npos || ctx.find("OBS") != std::string::npos,
          "Context contains memory type");

    mem.decayAll();
    check(mem.shortTermCount() <= 5, "Decay works");

    mem.summarizeLongTerm("Explored the forest area to the north");
    check(mem.longTermCount() >= 1, "Long-term summary stored");

    printf("  All memories:\n");
    for (auto& m : mem.getShortTerm()) {
        printf("    [%s] %s (importance=%.2f)\n",
               MemorySystem::memoryTypeToString(m.type).c_str(),
               m.content.c_str(), m.importance);
    }
}

void testGoalManager() {
    printf("\n=== Goal Manager ===\n");

    GoalManager gm;

    int g1 = gm.addGoal(GoalType::BUILD_SHELTER, "Build a wooden house", 0.8f, 1, 10, 5, 10);
    int g2 = gm.addGoal(GoalType::FIND_FOOD, "Find food to eat", 0.6f, 2);
    int g3 = gm.addGoal(GoalType::EXPLORE, "Explore the cave", 0.4f, 3);

    check(gm.activeCount() == 3, "3 active goals");

    Goal* top = gm.getActiveGoal();
    check(top != nullptr, "Has active goal");
    check(top->id == g1, "Highest priority goal first");
    check(top->type == GoalType::BUILD_SHELTER, "Top goal is BUILD_SHELTER");

    gm.updateProgress(g1, 0.5f);
    top = gm.getActiveGoal();
    check(top->progress == 0.5f, "Progress updated");

    gm.completeGoal(g1);
    check(gm.activeCount() == 2, "Completed goal removed from active");

    top = gm.getActiveGoal();
    check(top->id == g2, "Next priority goal is now top");

    std::string desc = gm.describeGoals();
    check(desc.find("FOOD") != std::string::npos, "Goal description contains type");
    check(desc.find("EXPLORE") != std::string::npos, "All active goals described");

    gm.clear();
    check(gm.activeCount() == 0, "Cleared all goals");
}

void testPerception() {
    printf("\n=== Perception System ===\n");

    VoxelOctree world;

    world.setBlock(10, 4, 10, BlockType::WOOD);
    world.setBlock(10, 5, 10, BlockType::WOOD);
    world.setBlock(10, 6, 10, BlockType::LEAVES);
    world.setBlock(12, 4, 12, BlockType::STONE);

    PerceptionSystem perc;
    perc.setWorld(&world);

    Observation obs = perc.observe(10.0f, 5.0f, 10.0f, 5.0f, 0, 293.15f);
    check(!obs.description.empty(), "Observation has description");
    check(obs.visibleBlocks.size() > 0, "Blocks visible");

    std::string formatted = perc.formatObservationForLLM(obs, 10, 5, 10, "TestBot", "builder");
    check(formatted.find("TestBot") != std::string::npos, "Formatted obs includes name");
    check(formatted.find("builder") != std::string::npos, "Formatted obs includes role");
    check(formatted.find("Position") != std::string::npos, "Formatted obs includes position");

    printf("  Observation: %.100s...\n", obs.description.c_str());
}

void testToolCalling() {
    printf("\n=== Tool Calling ===\n");

    VoxelOctree world;

    AgentToolCalling tools;
    tools.setWorld(&world);

    float x = 5, y = 5, z = 5;

    ToolResult r = tools.moveTo(x, y, z, 10, 5, 10);
    check(r.success, "moveTo succeeded");
    check((x != 5.0f || y != 5.0f || z != 5.0f), "Position changed");

    r = tools.placeBlock(8, 5, 8, "WOOD");
    check(r.success, "placeBlock succeeded");
    VoxelData vd;
    world.getBlock(8, 5, 8, vd);
    check(vd.type == BlockType::WOOD, "Block placed is WOOD");

    r = tools.breakBlock(8, 5, 8);
    check(r.success, "breakBlock succeeded");
    world.getBlock(8, 5, 8, vd);
    check(vd.type == BlockType::AIR || !vd.occupied, "Block removed");

    r = tools.searchFor(5, 5, 5, "STONE");
    check(!r.output.empty(), "searchFor returns a result message");

    r = tools.buildStructure("house", 6, 4, 6, 3);
    check(r.success, "buildStructure succeeded");
    world.getBlock(6, 4, 6, vd);
    check(vd.type == BlockType::WOOD, "Structure has WOOD blocks");

    std::string descs = AgentToolCalling::getToolDescriptions();
    check(descs.find("move_to") != std::string::npos, "Tool descriptions available");
}

void testCommunication() {
    printf("\n=== Agent Communication ===\n");

    AgentCommunication comm;

    Conversation conv = comm.startConversation(1, "Alice", 2, "Bob", "greeting", nullptr, 100);
    check(conv.id >= 0, "Conversation created");
    check(conv.lines.size() >= 2, "Mock dialogue has lines");
    check(conv.lines[0].speaker == "Alice" || conv.lines[0].speaker == "Bob",
          "First speaker is valid");
    check(!conv.lines[0].text.empty(), "Dialogue has text");

    printf("  Dialogue:\n");
    for (auto& line : conv.lines) {
        printf("    %s [%s]: %s\n", line.speaker.c_str(), line.emotion.c_str(), line.text.c_str());
    }

    MemorySystem mem;
    comm.recordConversation(conv, mem, 100);
    check(mem.shortTermCount() > 0, "Conversation recorded in memory");

    std::string gossip = comm.gossipToThirdParty(conv, "Charlie");
    check(!gossip.empty(), "Gossip generated");
}

void testCognitiveAgent() {
    printf("\n=== Cognitive Agent (Full Loop) ===\n");

    VoxelOctree world;

    world.setBlock(10, 4, 10, BlockType::WOOD);
    world.setBlock(11, 4, 10, BlockType::WOOD);
    world.setBlock(12, 4, 10, BlockType::WOOD);
    world.setBlock(10, 4, 11, BlockType::WOOD);
    world.setBlock(12, 4, 11, BlockType::WOOD);
    world.setBlock(10, 4, 12, BlockType::WOOD);
    world.setBlock(11, 4, 12, BlockType::WOOD);
    world.setBlock(12, 4, 12, BlockType::WOOD);
    world.setBlock(15, 4, 15, BlockType::STONE);
    world.setBlock(16, 4, 15, BlockType::COAL);
    world.setBlock(8, 3, 8, BlockType::GRASS);
    world.setBlock(9, 3, 9, BlockType::GRASS);

    MockLLMClient mock;
    LLMConfig cfg;
    mock.initialize(cfg);

    int actionIdx = 0;
    std::vector<std::string> responses = {
        R"({"tool":"place_block","args":["6","4","6","WOOD"],"reasoning":"Start building shelter floor"})",
        R"({"tool":"place_block","args":["7","4","6","WOOD"],"reasoning":"Continue shelter floor"})",
        R"({"tool":"place_block","args":["6","4","7","WOOD"],"reasoning":"More floor"})",
        R"({"tool":"place_block","args":["7","4","7","WOOD"],"reasoning":"Finish floor"})",
        R"({"tool":"place_block","args":["6","5","6","WOOD"],"reasoning":"Start wall"})",
        R"({"tool":"place_block","args":["7","5","6","WOOD"],"reasoning":"Wall section"})",
        R"({"tool":"place_block","args":["6","5","7","WOOD"],"reasoning":"Another wall"})",
        R"({"tool":"place_block","args":["7","5","7","WOOD"],"reasoning":"Last wall"})",
        R"({"tool":"search_for","args":["COAL"],"reasoning":"Need fuel for furnace"})",
        R"({"tool":"move_to","args":["15","4","15"],"reasoning":"Go to coal deposit"})",
    };

    mock.setResponseGenerator([&](const std::string& prompt, const std::string& sys) -> std::string {
        if (actionIdx < (int)responses.size()) {
            return responses[actionIdx++];
        }
        return R"({"tool":"idle","args":[],"reasoning":"All done"})";
    });

    CognitiveAgent agent;
    agent.init(0, "BuilderBob", AgentRole::BUILDER, &mock, &world, nullptr);

    agent.addGoal(GoalType::BUILD_SHELTER, "Build a small wooden house", 0.9f, 1);

    check(agent.getName() == "BuilderBob", "Agent name correct");
    check(agent.getRole() == AgentRole::BUILDER, "Agent role correct");
    check(agent.getIsAlive(), "Agent is alive");
    check(agent.getHealth() == 100.0f, "Full health");

    printf("  Running 10 ticks...\n");
    for (int i = 0; i < 10; i++) {
        agent.tick(i);
        printf("  Tick %2d: %s\n", i, agent.statusString().c_str());
    }

    check(agent.getIsAlive(), "Agent still alive after 10 ticks");
    check(agent.getMemory().shortTermCount() > 0, "Agent has memories");
    check(agent.getLastAction().type != "idle" || actionIdx >= (int)responses.size(),
          "Agent took actions");

    VoxelData vd;
    world.getBlock(6, 4, 6, vd);
    check(vd.type == BlockType::WOOD, "Agent placed WOOD blocks");

    world.getBlock(7, 5, 7, vd);
    check(vd.type == BlockType::WOOD, "Agent built walls");

    std::string ctx = agent.getMemory().buildContextString(5);
    check(!ctx.empty(), "Memory context available");

    printf("\n  Final status: %s\n", agent.statusString().c_str());
    printf("  Memory: %d short-term, %d long-term\n",
           agent.getMemory().shortTermCount(), agent.getMemory().longTermCount());
    printf("  Goals: %d active\n", agent.getGoals().activeCount());
}

int main() {
    printf("========================================\n");
    printf(" OpenMind Agent Cognitive Layer Tests\n");
    printf("========================================\n");

    testMemorySystem();
    testGoalManager();
    testPerception();
    testToolCalling();
    testCommunication();
    testCognitiveAgent();

    printf("\n========================================\n");
    printf(" Results: %d passed, %d failed\n", testsPassed, testsFailed);
    printf("========================================\n");

    return testsFailed > 0 ? 1 : 0;
}
