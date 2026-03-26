#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <nlohmann/json.hpp>

// Result of an HTTP POST to the Telegram Bot API
struct ApiResponse {
    bool ok       = false;
    int  httpCode = 0;
    nlohmann::json result;  // parsed "result" field when ok==true
    std::string    raw;     // full response body
};

// A single inline-keyboard button: {text, callback_data or url}
using BtnRow = std::vector<std::pair<std::string, std::string>>;

class TelegramApiClient
{
public:
    TelegramApiClient();
    ~TelegramApiClient();

    void setToken(const std::string& token);

    // All methods are synchronous (blocking) – call from a worker thread
    ApiResponse getUpdates(int offset, int timeout = 25);
    ApiResponse sendMessage(int64_t chatId, const std::string& text,
                            const nlohmann::json& replyMarkup = {});
    ApiResponse sendMessageWithInlineKeyboard(int64_t chatId,
                                              const std::string& text,
                                              const std::vector<BtnRow>& buttons);
    ApiResponse sendPhoto(int64_t chatId, const std::string& fileId,
                          const std::string& caption = {});
    ApiResponse sendDocument(int64_t chatId, const std::string& fileId,
                              const std::string& caption = {});
    ApiResponse forwardMessage(int64_t chatId, int64_t fromChatId, int messageId);
    ApiResponse answerCallbackQuery(const std::string& callbackQueryId,
                                    const std::string& text = {});
    ApiResponse sendToChannel(const std::string& channelId, const std::string& text);

    static nlohmann::json buildInlineKeyboardMarkup(const std::vector<BtnRow>& buttons);

private:
    ApiResponse post(const std::string& method, const nlohmann::json& params);

    void*       m_hSession = nullptr;  // HINTERNET – void* to avoid including winhttp.h here
    std::string m_token;
};
