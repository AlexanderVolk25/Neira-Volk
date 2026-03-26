#include "telegramapiclient.h"
#include "utils.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <winhttp.h>
#pragma comment(lib, "winhttp.lib")

#include <string>
#include <vector>

TelegramApiClient::TelegramApiClient()
{
    m_hSession = WinHttpOpen(
        L"NeiraBotPanel/1.0",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS,
        0);
}

TelegramApiClient::~TelegramApiClient()
{
    if (m_hSession)
        WinHttpCloseHandle(reinterpret_cast<HINTERNET>(m_hSession));
}

void TelegramApiClient::setToken(const std::string& token)
{
    m_token = token;
}

// ---------- Public API methods ----------

ApiResponse TelegramApiClient::getUpdates(int offset, int timeout)
{
    nlohmann::json params;
    params["offset"]  = offset;
    params["timeout"] = timeout;
    return post("getUpdates", params);
}

ApiResponse TelegramApiClient::sendMessage(int64_t chatId,
                                            const std::string& text,
                                            const nlohmann::json& replyMarkup)
{
    nlohmann::json params;
    params["chat_id"]    = chatId;
    params["text"]       = text;
    params["parse_mode"] = "HTML";
    if (!replyMarkup.is_null() && !replyMarkup.empty())
        params["reply_markup"] = replyMarkup;
    return post("sendMessage", params);
}

ApiResponse TelegramApiClient::sendMessageWithInlineKeyboard(
    int64_t chatId,
    const std::string& text,
    const std::vector<BtnRow>& buttons)
{
    nlohmann::json params;
    params["chat_id"]      = chatId;
    params["text"]         = text;
    params["parse_mode"]   = "HTML";
    params["reply_markup"] = buildInlineKeyboardMarkup(buttons);
    return post("sendMessage", params);
}

ApiResponse TelegramApiClient::sendPhoto(int64_t chatId,
                                          const std::string& fileId,
                                          const std::string& caption)
{
    nlohmann::json params;
    params["chat_id"]    = chatId;
    params["photo"]      = fileId;
    params["parse_mode"] = "HTML";
    if (!caption.empty()) params["caption"] = caption;
    return post("sendPhoto", params);
}

ApiResponse TelegramApiClient::sendDocument(int64_t chatId,
                                             const std::string& fileId,
                                             const std::string& caption)
{
    nlohmann::json params;
    params["chat_id"]    = chatId;
    params["document"]   = fileId;
    params["parse_mode"] = "HTML";
    if (!caption.empty()) params["caption"] = caption;
    return post("sendDocument", params);
}

ApiResponse TelegramApiClient::forwardMessage(int64_t chatId,
                                               int64_t fromChatId,
                                               int messageId)
{
    nlohmann::json params;
    params["chat_id"]      = chatId;
    params["from_chat_id"] = fromChatId;
    params["message_id"]   = messageId;
    return post("forwardMessage", params);
}

ApiResponse TelegramApiClient::answerCallbackQuery(const std::string& callbackQueryId,
                                                    const std::string& text)
{
    nlohmann::json params;
    params["callback_query_id"] = callbackQueryId;
    if (!text.empty()) params["text"] = text;
    return post("answerCallbackQuery", params);
}

ApiResponse TelegramApiClient::sendToChannel(const std::string& channelId,
                                              const std::string& text)
{
    nlohmann::json params;
    params["chat_id"]    = channelId;
    params["text"]       = text;
    params["parse_mode"] = "HTML";
    return post("sendMessage", params);
}

// ---------- Static helper ----------

nlohmann::json TelegramApiClient::buildInlineKeyboardMarkup(
    const std::vector<BtnRow>& buttons)
{
    nlohmann::json rows = nlohmann::json::array();
    for (const auto& row : buttons) {
        nlohmann::json rowArr = nlohmann::json::array();
        for (const auto& btn : row) {
            nlohmann::json b;
            b["text"] = btn.first;
            const std::string& data = btn.second;
            if (startsWith(data, "http://") || startsWith(data, "https://"))
                b["url"] = data;
            else
                b["callback_data"] = data;
            rowArr.push_back(b);
        }
        rows.push_back(rowArr);
    }
    nlohmann::json markup;
    markup["inline_keyboard"] = rows;
    return markup;
}

// ---------- Private HTTP post ----------

ApiResponse TelegramApiClient::post(const std::string& method,
                                     const nlohmann::json& params)
{
    ApiResponse resp;
    if (m_token.empty()) { resp.raw = "No token set"; return resp; }

    HINTERNET hSession = reinterpret_cast<HINTERNET>(m_hSession);

    // Connect to api.telegram.org (HTTPS, port 443)
    HINTERNET hConnect = WinHttpConnect(hSession, L"api.telegram.org",
                                        INTERNET_DEFAULT_HTTPS_PORT, 0);
    if (!hConnect) { resp.raw = "WinHttpConnect failed"; return resp; }

    // Build the URL path: /bot<TOKEN>/<METHOD>
    std::wstring path = L"/bot" + toWide(m_token) + L"/" + toWide(method);

    HINTERNET hRequest = WinHttpOpenRequest(
        hConnect, L"POST", path.c_str(), nullptr,
        WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES,
        WINHTTP_FLAG_SECURE);
    if (!hRequest) {
        WinHttpCloseHandle(hConnect);
        resp.raw = "WinHttpOpenRequest failed";
        return resp;
    }

    // Set timeouts: (resolve, connect, send, receive) — getUpdates uses 25-sec long-poll
    DWORD resolveTimeout  =  10000;
    DWORD connectTimeout  =  15000;
    DWORD sendTimeout     =  15000;
    DWORD receiveTimeout  = (method == "getUpdates") ? 35000 : 20000;
    WinHttpSetTimeouts(hRequest, resolveTimeout, connectTimeout,
                       sendTimeout, receiveTimeout);

    // Serialize body
    std::string body = params.dump();
    std::wstring contentType = L"Content-Type: application/json\r\n";

    BOOL sent = WinHttpSendRequest(
        hRequest,
        contentType.c_str(), static_cast<DWORD>(-1L),
        const_cast<char*>(body.c_str()), static_cast<DWORD>(body.size()),
        static_cast<DWORD>(body.size()), 0);

    if (!sent || !WinHttpReceiveResponse(hRequest, nullptr)) {
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        resp.raw = "Request failed";
        return resp;
    }

    // Read HTTP status code
    DWORD statusCode = 0;
    DWORD statusLen  = sizeof(statusCode);
    WinHttpQueryHeaders(hRequest,
        WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
        WINHTTP_HEADER_NAME_BY_INDEX, &statusCode, &statusLen,
        WINHTTP_NO_HEADER_INDEX);
    resp.httpCode = static_cast<int>(statusCode);

    // Read body
    std::string responseBody;
    DWORD available = 0;
    while (WinHttpQueryDataAvailable(hRequest, &available) && available > 0) {
        std::vector<char> buf(available + 1, 0);
        DWORD read = 0;
        if (WinHttpReadData(hRequest, buf.data(), available, &read))
            responseBody.append(buf.data(), read);
    }

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);

    resp.raw = responseBody;

    // Parse JSON
    try {
        auto doc = nlohmann::json::parse(responseBody);
        resp.ok = doc.value("ok", false);
        if (resp.ok && doc.contains("result"))
            resp.result = doc["result"];
    } catch (...) {
        resp.ok = false;
    }

    return resp;
}
