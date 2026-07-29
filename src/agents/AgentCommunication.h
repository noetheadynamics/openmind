#pragma once

#include "AgentCognitive.h"
#include "LLMInterface.h"
#include "MemorySystem.h"
#include <string>
#include <vector>
#include <sstream>

namespace OpenMind {

struct DialogueLine {
    std::string speaker;
    std::string text;
    std::string emotion;
};

struct Conversation {
    int id = 0;
    int agent1Id = 0;
    int agent2Id = 0;
    std::vector<DialogueLine> lines;
    float relationshipChange = 0.0f;
    uint64_t tick = 0;
};

class AgentCommunication {
public:
    AgentCommunication() = default;

    Conversation startConversation(int fromId, const std::string& fromName,
                                   int toId, const std::string& toName,
                                   const std::string& topic,
                                   LLMInterface* llm, uint64_t tick) {
        Conversation conv;
        conv.id = nextConvId++;
        conv.agent1Id = fromId;
        conv.agent2Id = toId;
        conv.tick = tick;

        if (!llm || !llm->isAvailable()) {
            conv.lines.push_back({fromName, "Hello there!", "neutral"});
            conv.lines.push_back({toName, "Hi!", "neutral"});
            return conv;
        }

        std::string system = "You are generating dialogue between two agents in a voxel world. "
                             "Respond with valid JSON: {\"dialogue\":[{\"speaker\":\"name\",\"text\":\"line\",\"emotion\":\"emotion\"}],"
                             "\"relationship_change\":-10 to 10}";

        std::ostringstream prompt;
        prompt << "Agent 1: " << fromName << "\n";
        prompt << "Agent 2: " << toName << "\n";
        prompt << "Topic: " << topic << "\n";
        prompt << "Generate 2-4 lines of dialogue.";

        LLMResponse resp = llm->sendPrompt(prompt.str(), system);

        if (resp.success) {
            conv.lines = parseDialogue(resp.content, fromName, toName);
        }

        if (conv.lines.empty()) {
            conv.lines.push_back({fromName, "Hello " + toName + "!", "neutral"});
            conv.lines.push_back({toName, "Hey " + fromName + "!", "neutral"});
        }

        return conv;
    }

    void recordConversation(const Conversation& conv, MemorySystem& mem, uint64_t tick) {
        for (auto& line : conv.lines) {
            std::string content = line.speaker + " said: \"" + line.text + "\"";
            mem.store(content, MemoryType::CONVERSATION, 0.6f, tick);
        }
    }

    std::string gossipToThirdParty(const Conversation& conv, const std::string& reporterName) const {
        if (conv.lines.empty()) return "";
        std::ostringstream ss;
        ss << reporterName << " overheard a conversation about ";
        if (!conv.lines.empty()) {
            ss << "something. They said: \"" << conv.lines[0].text << "\"";
        }
        return ss.str();
    }

    const std::vector<Conversation>& getConversations() const { return conversations; }

private:
    std::vector<DialogueLine> parseDialogue(const std::string& json,
                                            const std::string& name1,
                                            const std::string& name2) {
        std::vector<DialogueLine> lines;
        if (json.size() < 2) return lines;

        size_t dialogPos = json.find("\"dialogue\"");
        if (dialogPos == std::string::npos) return lines;

        size_t arrStart = json.find('[', dialogPos);
        if (arrStart == std::string::npos || arrStart >= json.size() - 1) return lines;

        size_t arrEnd = json.find(']', arrStart + 1);
        if (arrEnd == std::string::npos || arrEnd <= arrStart + 1) return lines;

        std::string arr = json.substr(arrStart + 1, arrEnd - arrStart - 1);

        size_t pos = 0;
        size_t maxSearch = arr.size();
        int maxLines = 100;
        while (pos < maxSearch && maxLines-- > 0) {
            size_t speakerPos = arr.find("\"speaker\"", pos);
            if (speakerPos == std::string::npos) break;

            DialogueLine line;

            size_t speakerVal = arr.find('"', speakerPos + 9);
            if (speakerVal != std::string::npos && speakerVal + 1 < arr.size()) {
                speakerVal++;
                size_t speakerEnd = arr.find('"', speakerVal);
                if (speakerEnd != std::string::npos) {
                    line.speaker = arr.substr(speakerVal, speakerEnd - speakerVal);
                }
            }

            size_t textPos = arr.find("\"text\"", pos);
            if (textPos != std::string::npos) {
                size_t textVal = arr.find('"', textPos + 6);
                if (textVal != std::string::npos && textVal + 1 < arr.size()) {
                    textVal++;
                    size_t textEnd = arr.find('"', textVal);
                    if (textEnd != std::string::npos) {
                        line.text = arr.substr(textVal, textEnd - textVal);
                    }
                }
            }

            size_t emoPos = arr.find("\"emotion\"", pos);
            if (emoPos != std::string::npos) {
                size_t emoVal = arr.find('"', emoPos + 9);
                if (emoVal != std::string::npos && emoVal + 1 < arr.size()) {
                    emoVal++;
                    size_t emoEnd = arr.find('"', emoVal);
                    if (emoEnd != std::string::npos) {
                        line.emotion = arr.substr(emoVal, emoEnd - emoVal);
                    }
                }
            }

            if (!line.text.empty()) {
                if (line.speaker.empty()) {
                    line.speaker = (lines.size() % 2 == 0) ? name1 : name2;
                }
                lines.push_back(line);
            }

            pos = (textPos != std::string::npos) ? textPos + 6 : speakerPos + 9;
        }

        return lines;
    }

    std::vector<Conversation> conversations;
    int nextConvId = 0;
};

} // namespace OpenMind
