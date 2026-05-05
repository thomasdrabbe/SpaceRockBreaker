#include <SFML/Graphics.hpp>
#include <windows.h>
#include <winhttp.h>

#include <atomic>
#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#pragma comment(lib, "winhttp.lib")

namespace fs = std::filesystem;

static std::wstring exeDir() {
    wchar_t buf[MAX_PATH]{};
    GetModuleFileNameW(nullptr, buf, MAX_PATH);
    std::wstring p(buf);
    const auto pos = p.find_last_of(L"\\/");
    if (pos == std::wstring::npos)
        return L".";
    return p.substr(0, pos);
}

static std::string trim(std::string s) {
    while (!s.empty() && (s.back() == '\r' || s.back() == '\n' || s.back() == ' '))
        s.pop_back();
    size_t i = 0;
    while (i < s.size() && (s[i] == ' ' || s[i] == '\t'))
        ++i;
    return s.substr(i);
}

static std::string readLocalVersion(const fs::path& dir) {
    const fs::path f = dir / "version.txt";
    std::ifstream    in(f);
    std::string      line;
    if (!std::getline(in, line))
        return "0.0.0";
    return trim(line);
}

static void logLine(const std::string& msg) {
    const fs::path log = fs::path(exeDir()) / "launcher_log.txt";
    std::ofstream  out(log, std::ios::app);
    out << msg << '\n';
}

static bool winHttpDownload(const std::wstring& host,
                            const std::wstring& urlPath,
                            INTERNET_PORT      port,
                            bool               https,
                            const fs::path&    destFile,
                            std::atomic<std::uint64_t>* downloadedBytes,
                            std::atomic<std::uint64_t>* totalBytes) {
    HINTERNET ses = WinHttpOpen(L"SpaceRockLauncher/1.0",
                                WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                                WINHTTP_NO_PROXY_NAME,
                                WINHTTP_NO_PROXY_BYPASS,
                                0);
    if (!ses)
        return false;
    DWORD redirect = WINHTTP_OPTION_REDIRECT_POLICY_ALWAYS;
    WinHttpSetOption(ses, WINHTTP_OPTION_REDIRECT_POLICY, &redirect,
                       sizeof(redirect));

    HINTERNET conn = WinHttpConnect(ses, host.c_str(), port, 0);
    if (!conn) {
        WinHttpCloseHandle(ses);
        return false;
    }

    DWORD flags = https ? WINHTTP_FLAG_SECURE : 0;
    HINTERNET req = WinHttpOpenRequest(
        conn, L"GET", urlPath.c_str(), nullptr, WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
    if (!req) {
        WinHttpCloseHandle(conn);
        WinHttpCloseHandle(ses);
        return false;
    }

    if (!WinHttpSendRequest(req, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                            WINHTTP_NO_REQUEST_DATA, 0, 0, 0)
        || !WinHttpReceiveResponse(req, nullptr)) {
        WinHttpCloseHandle(req);
        WinHttpCloseHandle(conn);
        WinHttpCloseHandle(ses);
        return false;
    }

    DWORD status = 0;
    DWORD sz     = sizeof(status);
    WinHttpQueryHeaders(req, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                        WINHTTP_HEADER_NAME_BY_INDEX, &status, &sz,
                        WINHTTP_NO_HEADER_INDEX);
    if (status != 200) {
        logLine("HTTP status: " + std::to_string(status));
        WinHttpCloseHandle(req);
        WinHttpCloseHandle(conn);
        WinHttpCloseHandle(ses);
        return false;
    }

    DWORD contentLen = 0;
    DWORD clSize     = sizeof(contentLen);
    if (WinHttpQueryHeaders(req,
                            WINHTTP_QUERY_CONTENT_LENGTH | WINHTTP_QUERY_FLAG_NUMBER,
                            WINHTTP_HEADER_NAME_BY_INDEX, &contentLen, &clSize,
                            WINHTTP_NO_HEADER_INDEX))
        totalBytes->store(contentLen);
    else
        totalBytes->store(0);

    std::ofstream out(destFile, std::ios::binary);
    if (!out) {
        WinHttpCloseHandle(req);
        WinHttpCloseHandle(conn);
        WinHttpCloseHandle(ses);
        return false;
    }

    std::uint64_t got = 0;
    for (;;) {
        DWORD avail = 0;
        if (!WinHttpQueryDataAvailable(req, &avail))
            break;
        if (avail == 0)
            break;
        std::vector<char> chunk(avail);
        DWORD read = 0;
        if (!WinHttpReadData(req, chunk.data(), avail, &read) || read == 0)
            break;
        out.write(chunk.data(), static_cast<std::streamsize>(read));
        got += read;
        downloadedBytes->store(got);
    }

    WinHttpCloseHandle(req);
    WinHttpCloseHandle(conn);
    WinHttpCloseHandle(ses);
    return out.good();
}

static std::string httpGetText(const std::wstring& host,
                               const std::wstring& urlPath,
                               bool                https) {
    const fs::path tmp =
        fs::temp_directory_path()
        / ("srb_ver_" + std::to_string(GetCurrentProcessId()) + ".txt");
    std::atomic<std::uint64_t> down{};
    std::atomic<std::uint64_t> tot{};
    if (!winHttpDownload(host, urlPath, https ? INTERNET_DEFAULT_HTTPS_PORT
                                             : INTERNET_DEFAULT_HTTP_PORT,
                         https, tmp, &down, &tot))
        return {};
    std::ifstream in(tmp, std::ios::binary);
    std::stringstream ss;
    ss << in.rdbuf();
    fs::remove(tmp);
    return trim(ss.str());
}

static bool runExpandArchive(const fs::path& zip, const fs::path& dest) {
    std::wstring cmd =
        L"powershell.exe -NoProfile -NonInteractive -Command "
        L"\"$ErrorActionPreference='Stop'; Expand-Archive -LiteralPath '";
    cmd += zip.wstring();
    cmd += L"' -DestinationPath '";
    cmd += dest.wstring();
    cmd += L"' -Force\"";

    STARTUPINFOW si{};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi{};
    std::vector<wchar_t> buf(cmd.begin(), cmd.end());
    buf.push_back(L'\0');
    if (!CreateProcessW(nullptr, buf.data(), nullptr, nullptr, FALSE, 0,
                        nullptr, nullptr, &si, &pi))
        return false;
    WaitForSingleObject(pi.hProcess, INFINITE);
    DWORD code = 1;
    GetExitCodeProcess(pi.hProcess, &code);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    return code == 0;
}

static void startGame(const fs::path& exe) {
    std::wstring p = exe.wstring();
    std::vector<wchar_t> args(p.begin(), p.end());
    args.push_back(L'\0');
    STARTUPINFOW si{};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi{};
    CreateProcessW(exe.c_str(), args.data(), nullptr, nullptr, FALSE, 0,
                   nullptr, exeDir().c_str(), &si, &pi);
    if (pi.hThread)
        CloseHandle(pi.hThread);
    if (pi.hProcess)
        CloseHandle(pi.hProcess);
}

int main() {
    const fs::path dir     = exeDir();
    const fs::path gameExe = dir / "SpaceRockBreaker.exe";

    const std::string localVer = readLocalVersion(dir);

    std::string remoteVer;
    try {
        remoteVer = httpGetText(
            L"raw.githubusercontent.com",
            LR"(/thomasdrabbe/SpaceRockBreaker/main/version.txt)",
            true);
    } catch (...) {
        remoteVer.clear();
    }

    if (remoteVer.empty()) {
        logLine("Remote version check failed; starting game.");
        if (fs::exists(gameExe))
            startGame(gameExe);
        return 0;
    }

    if (remoteVer == localVer) {
        if (fs::exists(gameExe))
            startGame(gameExe);
        return 0;
    }

    sf::RenderWindow window(
        sf::VideoMode(sf::Vector2u{ 400u, 150u }),
        sf::String{ L"Space Rock Breaker — Update" },
        sf::Style::Titlebar | sf::Style::Close);
    window.setFramerateLimit(60);

    sf::Font font;
    if (!font.openFromFile("C:/Windows/Fonts/arial.ttf"))
        (void)font.openFromFile("C:/Windows/Fonts/segoeui.ttf");

    std::atomic<std::uint64_t> downloaded{ 0 };
    std::atomic<std::uint64_t> total{ 0 };
    std::atomic<int>           status{ 0 };
    const fs::path             zipPath =
        fs::temp_directory_path()
        / ("SpaceRockBreaker_update_" + std::to_string(GetCurrentProcessId())
           + ".zip");

    std::thread worker([&]() {
        bool ok = false;

        // 1) Normale distributie via GitHub Releases (aanbevolen).
        ok = winHttpDownload(
            L"github.com",
            LR"(/thomasdrabbe/SpaceRockBreaker/releases/latest/download/SpaceRockBreaker.zip)",
            INTERNET_DEFAULT_HTTPS_PORT, true, zipPath, &downloaded, &total);

        // 2) Fallback: direct bestand in repository root.
        if (!ok) {
            downloaded.store(0);
            total.store(0);
            ok = winHttpDownload(
                L"raw.githubusercontent.com",
                LR"(/thomasdrabbe/SpaceRockBreaker/main/SpaceRockBreaker.zip)",
                INTERNET_DEFAULT_HTTPS_PORT, true, zipPath, &downloaded, &total);
            if (ok)
                logLine("Updater fallback gebruikt: raw main/SpaceRockBreaker.zip");
        }

        // 3) Fallback: zip in installer_output map in de repo.
        if (!ok) {
            downloaded.store(0);
            total.store(0);
            ok = winHttpDownload(
                L"raw.githubusercontent.com",
                LR"(/thomasdrabbe/SpaceRockBreaker/main/installer_output/SpaceRockBreaker.zip)",
                INTERNET_DEFAULT_HTTPS_PORT, true, zipPath, &downloaded, &total);
            if (ok)
                logLine("Updater fallback gebruikt: raw installer_output/SpaceRockBreaker.zip");
        }

        if (!ok) {
            status.store(-1);
            return;
        }
        if (!runExpandArchive(zipPath, dir)) {
            status.store(-2);
            fs::remove(zipPath);
            return;
        }
        fs::remove(zipPath);
        std::ofstream verOut(dir / "version.txt");
        verOut << remoteVer << '\n';
        if (!fs::exists(gameExe))
            status.store(-3);
        else
            status.store(1);
    });

    sf::Clock clock;
    bool        workerJoined = false;
    while (window.isOpen()) {
        while (const std::optional ev = window.pollEvent()) {
            if (ev->is<sf::Event::Closed>()) {
                if (!workerJoined && worker.joinable()) {
                    worker.detach();
                    workerJoined = true;
                }
                window.close();
            }
        }

        const int st = status.load();
        if (!workerJoined && st != 0) {
            worker.join();
            workerJoined = true;
            if (st == 1 && fs::exists(gameExe)) {
                startGame(gameExe);
                return 0;
            }
            if (st == -1 || st == -2)
                logLine("Update failed (download or extract).");
        }

        window.clear(sf::Color(20, 22, 34));

        std::string line =
            "Update beschikbaar: v" + remoteVer + " — Bezig met downloaden...";
        if (workerJoined) {
            if (st == -1)
                line = "Download mislukt (geen release/zip gevonden). Sluit dit venster.";
            else if (st == -2)
                line = "Uitpakken mislukt. Sluit dit venster.";
            else if (st == -3)
                line = "SpaceRockBreaker.exe niet gevonden na update.";
        }

        sf::Text txt(font);
        txt.setCharacterSize(14);
        txt.setString(line);
        txt.setFillColor(sf::Color(230, 235, 255));
        txt.setPosition({ 12.f, 18.f });
        window.draw(txt);

        const std::uint64_t t  = total.load();
        const std::uint64_t d  = downloaded.load();
        float               p0 = (t > 0)
            ? std::min(1.f,
                       static_cast<float>(static_cast<double>(d)
                                          / static_cast<double>(t)))
            : 0.f;
        if (st == 0 && t == 0)
            p0 = std::min(1.f, clock.getElapsedTime().asSeconds() / 8.f);

        sf::RectangleShape track(sf::Vector2f{ 360.f, 18.f });
        track.setPosition({ 20.f, 95.f });
        track.setFillColor(sf::Color(40, 44, 70));
        track.setOutlineColor(sf::Color(90, 100, 150));
        track.setOutlineThickness(1.f);
        window.draw(track);

        sf::RectangleShape fill(sf::Vector2f{ 356.f * p0, 14.f });
        fill.setPosition({ 22.f, 97.f });
        fill.setFillColor(sf::Color(80, 180, 255));
        window.draw(fill);

        window.display();
    }

    if (!workerJoined && worker.joinable())
        worker.join();
    if (fs::exists(gameExe))
        startGame(gameExe);
    return 0;
}
