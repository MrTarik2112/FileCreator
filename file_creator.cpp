#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <iomanip>
#include <chrono>
#include <sstream>
#include <thread>
#include <mutex>
#include <atomic>
#include <algorithm>
#include <cmath>
#include <memory>

#ifdef _WIN32
#include <windows.h>
#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif
#endif

using namespace std;
using namespace chrono;

// ANSI Escape Codes
namespace ANSI {
    const string CLEAR_LINE = "\033[2K";
    const string CLEAR_SCREEN = "\033[2J";
    const string CURSOR_HOME = "\033[H";
    const string HIDE_CURSOR = "\033[?25l";
    const string SHOW_CURSOR = "\033[?25h";
    const string SAVE_CURSOR = "\033[s";
    const string RESTORE_CURSOR = "\033[u";
    
    string moveCursor(int row, int col) {
        return "\033[" + to_string(row) + ";" + to_string(col) + "H";
    }
    
    string moveUp(int lines) {
        return "\033[" + to_string(lines) + "A";
    }
    
    string moveDown(int lines) {
        return "\033[" + to_string(lines) + "B";
    }
}

namespace Color {
    const string RESET = "\033[0m";
    const string BOLD = "\033[1m";
    const string DIM = "\033[2m";
    
    const string RED = "\033[31m";
    const string GREEN = "\033[32m";
    const string YELLOW = "\033[33m";
    const string BLUE = "\033[34m";
    const string MAGENTA = "\033[35m";
    const string CYAN = "\033[36m";
    const string WHITE = "\033[37m";
    
    const string BG_RED = "\033[41m";
    const string BG_GREEN = "\033[42m";
    const string BG_YELLOW = "\033[43m";
    const string BG_BLUE = "\033[44m";
    const string BG_MAGENTA = "\033[45m";
    const string BG_CYAN = "\033[46m";
    
    const string BRIGHT_BLACK = "\033[90m";
    const string BRIGHT_RED = "\033[91m";
    const string BRIGHT_GREEN = "\033[92m";
    const string BRIGHT_YELLOW = "\033[93m";
    const string BRIGHT_BLUE = "\033[94m";
    const string BRIGHT_MAGENTA = "\033[95m";
    const string BRIGHT_CYAN = "\033[96m";
    const string BRIGHT_WHITE = "\033[97m";
    
    const string BG_BRIGHT_RED = "\033[101m";
    const string BG_BRIGHT_GREEN = "\033[102m";
    const string BG_BRIGHT_YELLOW = "\033[103m";
    const string BG_BRIGHT_BLUE = "\033[104m";
    const string BG_BRIGHT_MAGENTA = "\033[105m";
    const string BG_BRIGHT_CYAN = "\033[106m";
}

// Advanced Statistics
struct AdvancedStats {
    atomic<long long> totalBytes{0};
    atomic<double> peakSpeed{0};
    atomic<long long> operationCount{0};
    vector<double> speedSamples;
    mutex statsMutex;
    double avgSpeed = 0;
    double efficiency = 0;
    
    void recordSpeed(double speed) {
        lock_guard<mutex> lock(statsMutex);
        speedSamples.push_back(speed);
        
        double peak = peakSpeed.load();
        while (speed > peak && !peakSpeed.compare_exchange_weak(peak, speed));
        
        if (!speedSamples.empty()) {
            double sum = 0;
            int samples = min(static_cast<int>(speedSamples.size()), 50);
            for (int i = speedSamples.size() - samples; i < speedSamples.size(); i++) {
                sum += speedSamples[i];
            }
            avgSpeed = sum / samples;
        }
    }
};

// Display Manager - Prevents stacking
class DisplayManager {
private:
    int progressLine = 0;
    bool isInitialized = false;
    mutex displayMutex;
    
public:
    void initialize() {
        lock_guard<mutex> lock(displayMutex);
        if (!isInitialized) {
            cout << ANSI::HIDE_CURSOR;
            isInitialized = true;
        }
    }
    
    void cleanup() {
        lock_guard<mutex> lock(displayMutex);
        if (isInitialized) {
            cout << ANSI::SHOW_CURSOR;
            isInitialized = false;
        }
    }
    
    void clearProgress() {
        lock_guard<mutex> lock(displayMutex);
        cout << "\r" << ANSI::CLEAR_LINE << flush;
    }
    
    void updateProgress(const string& content) {
        lock_guard<mutex> lock(displayMutex);
        cout << "\r" << ANSI::CLEAR_LINE << content << flush;
    }
    
    void newLine() {
        lock_guard<mutex> lock(displayMutex);
        cout << "\n";
    }
};

// Turbocharged File Creator
class TurboFileCreator {
private:
    string fileName;
    long long fileSize;
    int numThreads;
    int bufferSizeMB;
    
    atomic<long long> bytesWritten{0};
    atomic<bool> errorFlag{false};
    atomic<int> activeWorkers{0};
    
    AdvancedStats stats;
    DisplayManager display;
    bool useRandomData;
    bool turboMode;
    
    void setupConsole() {
        #ifdef _WIN32
        SetConsoleOutputCP(CP_UTF8);
        SetConsoleCP(CP_UTF8);
        
        HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
        if (hOut != INVALID_HANDLE_VALUE) {
            DWORD dwMode = 0;
            if (GetConsoleMode(hOut, &dwMode)) {
                dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
                SetConsoleMode(hOut, dwMode);
            }
        }
        #endif
    }

    string formatBytes(long long bytes) const {
        const char* units[] = {"B", "KB", "MB", "GB", "TB", "PB"};
        int idx = 0;
        double size = static_cast<double>(bytes);
        
        while (size >= 1024.0 && idx < 5) {
            size /= 1024.0;
            idx++;
        }
        
        stringstream ss;
        ss << fixed << setprecision(2) << size << " " << units[idx];
        return ss.str();
    }

    string formatDuration(double seconds) const {
        if (seconds < 0.001) return to_string(static_cast<int>(seconds * 1000000)) + " μs";
        if (seconds < 1.0) return to_string(static_cast<int>(seconds * 1000)) + " ms";
        if (seconds < 60.0) {
            stringstream ss;
            ss << fixed << setprecision(3) << seconds << " sec";
            return ss.str();
        }
        if (seconds < 3600.0) {
            return to_string(static_cast<int>(seconds / 60)) + "m " + 
                   to_string(static_cast<int>(seconds) % 60) + "s";
        }
        return to_string(static_cast<int>(seconds / 3600)) + "h " + 
               to_string(static_cast<int>(seconds / 60) % 60) + "m";
    }

    string getSpeedIndicator(double currentSpeed, double avgSpeed) const {
        double ratio = avgSpeed > 0 ? currentSpeed / avgSpeed : 1.0;
        
        if (ratio >= 1.8) return Color::BRIGHT_GREEN + "🚀 BLAZING" + Color::RESET;
        if (ratio >= 1.4) return Color::BRIGHT_GREEN + "⚡ TURBO" + Color::RESET;
        if (ratio >= 1.1) return Color::GREEN + "✓ FAST" + Color::RESET;
        if (ratio >= 0.9) return Color::YELLOW + "➜ NORMAL" + Color::RESET;
        if (ratio >= 0.7) return Color::BRIGHT_RED + "⚠ SLOW" + Color::RESET;
        return Color::RED + "✗ CRITICAL" + Color::RESET;
    }

    string generateProgressBar(float progress, int width = 50) const {
        int filled = static_cast<int>(width * progress);
        stringstream bar;
        
        for (int i = 0; i < width; i++) {
            if (i < filled) {
                if (progress < 0.25) bar << Color::BG_BRIGHT_RED << " " << Color::RESET;
                else if (progress < 0.50) bar << Color::BG_BRIGHT_YELLOW << " " << Color::RESET;
                else if (progress < 0.75) bar << Color::BG_BRIGHT_CYAN << " " << Color::RESET;
                else bar << Color::BG_BRIGHT_GREEN << " " << Color::RESET;
            } else if (i == filled && filled < width) {
                bar << Color::BRIGHT_WHITE << "▶" << Color::RESET;
            } else {
                bar << Color::BRIGHT_BLACK << "━" << Color::RESET;
            }
        }
        
        return bar.str();
    }

    string generateSparkline(const vector<double>& data, int width = 20) const {
        if (data.empty()) return "";
        
        const string blocks[] = {"▁", "▂", "▃", "▄", "▅", "▆", "▇", "█"};
        stringstream spark;
        
        double maxVal = *max_element(data.begin(), data.end());
        double minVal = *min_element(data.begin(), data.end());
        double range = maxVal - minVal;
        
        if (range == 0) range = 1.0;
        
        int dataSize = data.size();
        int step = max(1, dataSize / width);
        
        spark << Color::BRIGHT_CYAN;
        for (int i = 0; i < min(width, dataSize); i += step) {
            int blockIndex = static_cast<int>(((data[i] - minVal) / range) * 7);
            spark << blocks[min(blockIndex, 7)];
        }
        spark << Color::RESET;
        
        return spark.str();
    }

    void displayProgress(long long current, long long total, double elapsed) {
        float progress = static_cast<float>(current) / static_cast<float>(total);
        double currentSpeed = elapsed > 0 ? current / elapsed : 0;
        
        stats.recordSpeed(currentSpeed);
        
        stringstream output;
        
        // Main progress bar
        output << Color::BRIGHT_WHITE << "  ┃ " << Color::RESET;
        output << generateProgressBar(progress, 45);
        output << Color::BRIGHT_WHITE << " ┃" << Color::RESET;
        
        // Percentage
        output << " ";
        if (progress < 0.33) output << Color::BRIGHT_RED;
        else if (progress < 0.66) output << Color::BRIGHT_YELLOW;
        else output << Color::BRIGHT_GREEN;
        output << Color::BOLD << setw(6) << fixed << setprecision(2) << (progress * 100.0) << "%" << Color::RESET;
        
        // Data transferred
        output << Color::BRIGHT_WHITE << " │ " << Color::RESET;
        output << Color::BRIGHT_CYAN << formatBytes(current) << Color::RESET;
        output << Color::BRIGHT_BLACK << "/" << Color::RESET;
        output << Color::WHITE << formatBytes(total) << Color::RESET;
        
        if (elapsed > 0.05) {
            // Speed
            output << Color::BRIGHT_WHITE << " │ " << Color::RESET;
            output << Color::BRIGHT_YELLOW << "⚡" << formatBytes(static_cast<long long>(currentSpeed)) << "/s" << Color::RESET;
            
            // Status
            output << Color::BRIGHT_WHITE << " │ " << Color::RESET;
            output << getSpeedIndicator(currentSpeed, stats.avgSpeed);
            
            // Workers
            output << Color::BRIGHT_WHITE << " │ " << Color::RESET;
            output << Color::BRIGHT_MAGENTA << "🔥" << activeWorkers << "/" << numThreads << Color::RESET;
            
            // ETA
            if (progress > 0.005 && currentSpeed > 0) {
                double eta = (total - current) / currentSpeed;
                output << Color::BRIGHT_WHITE << " │ " << Color::RESET;
                output << Color::BRIGHT_BLUE << "⏱" << formatDuration(eta) << Color::RESET;
            }
        }
        
        display.updateProgress(output.str());
    }

    void showBanner() {
        cout << "\n";
        cout << Color::BOLD << Color::BRIGHT_CYAN;
        cout << "  ╔═══════════════════════════════════════════════════════════════════════╗\n";
        cout << "  ║                                                                       ║\n";
        cout << "  ║           " << Color::BRIGHT_WHITE << "⚡⚡⚡ TURBO FILE CREATOR " << Color::BRIGHT_YELLOW << "v5.1 " << Color::BRIGHT_WHITE << "⚡⚡⚡" << Color::BRIGHT_CYAN << "              ║\n";
        cout << "  ║                                                                       ║\n";
        cout << "  ║     " << Color::BRIGHT_GREEN << "Ultra-Fast" << Color::BRIGHT_BLACK << " • " 
             << Color::BRIGHT_MAGENTA << "Multi-Threaded" << Color::BRIGHT_BLACK << " • "
             << Color::BRIGHT_YELLOW << "Optimized I/O" << Color::BRIGHT_BLACK << " • "
             << Color::BRIGHT_CYAN << "Pro Stats" << Color::BRIGHT_CYAN << "     ║\n";
        cout << "  ║                                                                       ║\n";
        cout << "  ╚═══════════════════════════════════════════════════════════════════════╝\n";
        cout << Color::RESET << "\n";
    }

    void showConfig() {
        cout << Color::BRIGHT_WHITE << "  ╭─ " << Color::BRIGHT_CYAN << "⚙ CONFIGURATION" << Color::BRIGHT_WHITE << " ──────────────────────────────────────────────╮\n" << Color::RESET;
        cout << Color::BRIGHT_WHITE << "  │\n";
        cout << "  │  " << Color::BRIGHT_WHITE << "📁 Target File    : " << Color::BRIGHT_GREEN << fileName << Color::RESET << "\n";
        cout << Color::BRIGHT_WHITE << "  │  " << Color::BRIGHT_WHITE << "📏 File Size      : " << Color::BRIGHT_YELLOW << formatBytes(fileSize) << Color::RESET << "\n";
        cout << Color::BRIGHT_WHITE << "  │  " << Color::BRIGHT_WHITE << "💾 Buffer Size    : " << Color::BRIGHT_BLUE << bufferSizeMB << " MB per thread" << Color::RESET << "\n";
        cout << Color::BRIGHT_WHITE << "  │  " << Color::BRIGHT_WHITE << "🔥 Thread Count   : " << Color::BRIGHT_MAGENTA << numThreads << " workers" << Color::RESET << "\n";
        cout << Color::BRIGHT_WHITE << "  │  " << Color::BRIGHT_WHITE << "🎨 Data Pattern   : " << (useRandomData ? Color::BRIGHT_GREEN + "Random" : Color::BRIGHT_CYAN + "Zero-filled") << Color::RESET << "\n";
        cout << Color::BRIGHT_WHITE << "  │  " << Color::BRIGHT_WHITE << "⚡ Turbo Mode     : " << (turboMode ? Color::BRIGHT_GREEN + "ENABLED ✓" : Color::YELLOW + "Standard") << Color::RESET << "\n";
        cout << Color::BRIGHT_WHITE << "  │  " << Color::BRIGHT_WHITE << "🚀 Optimization   : " << Color::BRIGHT_CYAN << "Maximum Performance" << Color::RESET << "\n";
        cout << Color::BRIGHT_WHITE << "  │\n";
        cout << "  ╰───────────────────────────────────────────────────────────────────╯\n";
        cout << Color::RESET << "\n";
    }

    void showDetailedStats(double totalTime) {
        stats.efficiency = stats.avgSpeed > 0 ? (stats.avgSpeed / stats.peakSpeed.load()) * 100 : 0;
        
        display.clearProgress();
        display.newLine();
        
        cout << "\n";
        cout << Color::BOLD << Color::BRIGHT_GREEN;
        cout << "  ╔═══════════════════════════════════════════════════════════════════════╗\n";
        cout << "  ║                                                                       ║\n";
        cout << "  ║                   " << Color::BRIGHT_WHITE << "✅ OPERATION COMPLETED ✅" << Color::BRIGHT_GREEN << "                        ║\n";
        cout << "  ║                                                                       ║\n";
        cout << "  ╚═══════════════════════════════════════════════════════════════════════╝\n";
        cout << Color::RESET << "\n";
        
        cout << Color::BRIGHT_CYAN << "  ╭─ " << Color::BRIGHT_WHITE << "📊 PERFORMANCE METRICS" << Color::BRIGHT_CYAN << " ────────────────────────────────────────╮\n" << Color::RESET;
        cout << Color::BRIGHT_WHITE << "  │\n";
        
        // File info
        cout << "  │  " << Color::BRIGHT_WHITE << "✓ File Created      : " << Color::BRIGHT_GREEN << fileName << Color::RESET << "\n";
        cout << "  │  " << Color::BRIGHT_WHITE << "✓ Total Size        : " << Color::BRIGHT_YELLOW << formatBytes(fileSize) << Color::RESET << "\n";
        cout << "  │  " << Color::BRIGHT_WHITE << "✓ Elapsed Time      : " << Color::BRIGHT_BLUE << formatDuration(totalTime) << Color::RESET << "\n";
        
        cout << Color::BRIGHT_WHITE << "  │\n";
        
        // Speed metrics
        double finalSpeed = totalTime > 0 ? fileSize / totalTime : 0;
        cout << "  │  " << Color::BRIGHT_WHITE << "⚡ Final Speed       : " << Color::BRIGHT_CYAN << formatBytes(static_cast<long long>(finalSpeed)) << "/s" << Color::RESET << "\n";
        cout << "  │  " << Color::BRIGHT_WHITE << "⚡ Average Speed     : " << Color::BRIGHT_YELLOW << formatBytes(static_cast<long long>(stats.avgSpeed)) << "/s" << Color::RESET << "\n";
        cout << "  │  " << Color::BRIGHT_WHITE << "🚀 Peak Speed        : " << Color::BRIGHT_MAGENTA << formatBytes(static_cast<long long>(stats.peakSpeed.load())) << "/s" << Color::RESET << "\n";
        
        cout << Color::BRIGHT_WHITE << "  │\n";
        
        // Thread info
        cout << "  │  " << Color::BRIGHT_WHITE << "🔥 Threads Used      : " << Color::BRIGHT_MAGENTA << numThreads << " workers" << Color::RESET << "\n";
        cout << "  │  " << Color::BRIGHT_WHITE << "💾 Buffer per Thread : " << Color::BRIGHT_BLUE << bufferSizeMB << " MB" << Color::RESET << "\n";
        cout << "  │  " << Color::BRIGHT_WHITE << "📊 Total Operations  : " << Color::BRIGHT_CYAN << stats.operationCount.load() << Color::RESET << "\n";
        
        cout << Color::BRIGHT_WHITE << "  │\n";
        
        // Efficiency
        cout << "  │  " << Color::BRIGHT_WHITE << "📈 Thread Efficiency : ";
        if (stats.efficiency > 85) cout << Color::BRIGHT_GREEN;
        else if (stats.efficiency > 70) cout << Color::BRIGHT_YELLOW;
        else cout << Color::BRIGHT_RED;
        cout << fixed << setprecision(1) << stats.efficiency << "%" << Color::RESET;
        
        if (stats.efficiency > 90) cout << Color::BRIGHT_GREEN << " ★ EXCELLENT" << Color::RESET;
        else if (stats.efficiency > 80) cout << Color::GREEN << " ✓ GREAT" << Color::RESET;
        else if (stats.efficiency > 70) cout << Color::YELLOW << " ➜ GOOD" << Color::RESET;
        cout << "\n";
        
        // Speed rating
        double speedMBps = finalSpeed / (1024.0 * 1024.0);
        cout << "  │  " << Color::BRIGHT_WHITE << "🏆 Performance       : ";
        if (speedMBps > 500) cout << Color::BRIGHT_GREEN << "EXTREME" << Color::RESET;
        else if (speedMBps > 200) cout << Color::BRIGHT_CYAN << "EXCELLENT" << Color::RESET;
        else if (speedMBps > 100) cout << Color::GREEN << "VERY GOOD" << Color::RESET;
        else if (speedMBps > 50) cout << Color::YELLOW << "GOOD" << Color::RESET;
        else cout << Color::RED << "STANDARD" << Color::RESET;
        cout << " (" << fixed << setprecision(0) << speedMBps << " MB/s)\n";
        
        // Sparkline
        if (!stats.speedSamples.empty()) {
            cout << Color::BRIGHT_WHITE << "  │\n";
            cout << "  │  " << Color::BRIGHT_WHITE << "📈 Speed History     : ";
            cout << generateSparkline(stats.speedSamples, 35) << "\n";
        }
        
        cout << Color::BRIGHT_WHITE << "  │\n";
        cout << "  ╰───────────────────────────────────────────────────────────────────╯\n";
        cout << Color::RESET << "\n";
    }

    void turboWriter(int threadID, long long startPos, long long endPos) {
        try {
            activeWorkers++;
            
            size_t bufSize = static_cast<size_t>(bufferSizeMB) * 1024 * 1024;
            vector<char> buffer(bufSize);
            
            if (useRandomData) {
                unsigned int seed = threadID * 123456789;
                for (size_t i = 0; i < bufSize; i += 8) {
                    seed = seed * 1103515245 + 12345;
                    *reinterpret_cast<unsigned int*>(&buffer[i]) = seed;
                }
            }
            
            ofstream file(fileName, ios::binary | ios::in | ios::out);
            if (!file) {
                errorFlag = true;
                activeWorkers--;
                return;
            }
            
            file.rdbuf()->pubsetbuf(nullptr, 0);
            file.seekp(startPos);
            long long pos = startPos;
            
            while (pos < endPos && !errorFlag) {
                long long remaining = endPos - pos;
                long long writeSize = min(static_cast<long long>(bufSize), remaining);
                
                file.write(buffer.data(), writeSize);
                
                if (!file.good()) {
                    errorFlag = true;
                    activeWorkers--;
                    return;
                }
                
                pos += writeSize;
                bytesWritten += writeSize;
                stats.operationCount++;
            }
            
            file.close();
            activeWorkers--;
            
        } catch (...) {
            errorFlag = true;
            activeWorkers--;
        }
    }

public:
    TurboFileCreator(const string& filename, long long size, int threads = 4, 
                     int bufferMB = 32, bool randomData = false, bool turbo = true) 
        : fileName(filename), fileSize(size), numThreads(threads), 
          bufferSizeMB(bufferMB), useRandomData(randomData), turboMode(turbo) {
        setupConsole();
        
        if (numThreads <= 0) {
            numThreads = max(1u, thread::hardware_concurrency());
        }
        
        if (turboMode) {
            bufferSizeMB = max(bufferSizeMB, 32);
            numThreads = max(numThreads, 4);
        }
    }

    bool execute() {
        showBanner();
        showConfig();
        
        cout << Color::BRIGHT_CYAN << "  ⚡ Initializing turbo file creation...\n" << Color::RESET;
        cout << Color::BRIGHT_BLACK << "  ═══════════════════════════════════════════════════════════════════\n" << Color::RESET;
        cout << "\n";

        // Pre-allocate file
        {
            ofstream file(fileName, ios::binary);
            if (!file) {
                cout << Color::BRIGHT_RED << "  ❌ ERROR: Cannot create file!\n" << Color::RESET;
                return false;
            }
            
            file.seekp(fileSize - 1);
            file.write("", 1);
            file.close();
        }

        // Initialize display
        display.initialize();

        auto startTime = steady_clock::now();

        // Launch workers
        vector<thread> workers;
        long long chunkSize = fileSize / numThreads;
        
        for (int i = 0; i < numThreads; i++) {
            long long start = i * chunkSize;
            long long end = (i == numThreads - 1) ? fileSize : (i + 1) * chunkSize;
            workers.emplace_back(&TurboFileCreator::turboWriter, this, i, start, end);
        }

        // Monitor progress
        while (bytesWritten < fileSize && !errorFlag) {
            this_thread::sleep_for(milliseconds(50));
            
            auto now = steady_clock::now();
            double elapsed = duration<double>(now - startTime).count();
            
            displayProgress(bytesWritten.load(), fileSize, elapsed);
        }

        // Wait for workers
        for (auto& worker : workers) {
            if (worker.joinable()) worker.join();
        }

        auto endTime = steady_clock::now();
        double totalTime = duration<double>(endTime - startTime).count();

        // Cleanup display
        display.cleanup();

        if (errorFlag) {
            cout << Color::BRIGHT_RED << "\n\n  ❌ ERROR: Write operation failed!\n" << Color::RESET;
            return false;
        }

        // Final progress
        displayProgress(fileSize, fileSize, totalTime);
        
        showDetailedStats(totalTime);
        return true;
    }
};

long long parseSizeInput(const string& input) {
    stringstream ss(input);
    double value;
    string unit;
    
    ss >> value;
    if (ss.eof()) return static_cast<long long>(value);
    
    ss >> unit;
    transform(unit.begin(), unit.end(), unit.begin(), ::toupper);
    
    if (unit == "B" || unit == "BYTE" || unit == "BYTES") return static_cast<long long>(value);
    if (unit == "KB") return static_cast<long long>(value * 1024);
    if (unit == "MB") return static_cast<long long>(value * 1024 * 1024);
    if (unit == "GB") return static_cast<long long>(value * 1024 * 1024 * 1024);
    if (unit == "TB") return static_cast<long long>(value * 1024LL * 1024LL * 1024LL * 1024LL);
    if (unit == "PB") return static_cast<long long>(value * 1024LL * 1024LL * 1024LL * 1024LL * 1024LL);
    
    return static_cast<long long>(value * 1024 * 1024);
}

int main(int argc, char* argv[]) {
    #ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    #endif
    
    string fileName;
    string sizeInput;
    int threads = 0;
    int bufferMB = 32;
    bool randomData = false;
    bool turbo = true;
    
    if (argc >= 3) {
        fileName = argv[1];
        sizeInput = string(argv[2]) + (argc > 3 ? " " + string(argv[3]) : "");
        
        if (argc >= 5) {
            threads = atoi(argv[4]);
            if (threads < 1) threads = 1;
            if (threads > 64) threads = 64;
        }
        
        if (argc >= 6) {
            bufferMB = atoi(argv[5]);
            if (bufferMB < 1) bufferMB = 16;
            if (bufferMB > 256) bufferMB = 256;
        }
        
        if (argc >= 7 && string(argv[6]) == "--random") {
            randomData = true;
        }
    } else {
        cout << "\n";
        cout << Color::BOLD << Color::BRIGHT_MAGENTA;
        cout << "  ╔═══════════════════════════════════════════════════════════════════════╗\n";
        cout << "  ║                      " << Color::BRIGHT_WHITE << "⚙️  TURBO SETUP WIZARD  ⚙️" << Color::BRIGHT_MAGENTA << "                      ║\n";
        cout << "  ╚═══════════════════════════════════════════════════════════════════════╝\n";
        cout << Color::RESET << "\n";
        
        cout << Color::BRIGHT_CYAN << "  📝 Filename: " << Color::RESET;
        getline(cin, fileName);
        
        cout << Color::BRIGHT_CYAN << "  📏 File size " << Color::BRIGHT_BLACK << "(e.g., 100 MB, 1 GB, 10 GB): " << Color::RESET;
        getline(cin, sizeInput);
        
        cout << Color::BRIGHT_CYAN << "  🔥 Thread count " << Color::BRIGHT_BLACK << "[1-64] (0=auto): " << Color::RESET;
        string threadInput;
        getline(cin, threadInput);
        if (!threadInput.empty()) {
            threads = atoi(threadInput.c_str());
            if (threads < 0) threads = 0;
            if (threads > 64) threads = 64;
        }
        
        cout << Color::BRIGHT_CYAN << "  💾 Buffer size per thread (MB) " << Color::BRIGHT_BLACK << "[16-256, default=32]: " << Color::RESET;
        string bufferInput;
        getline(cin, bufferInput);
        if (!bufferInput.empty()) {
            bufferMB = atoi(bufferInput.c_str());
            if (bufferMB < 16) bufferMB = 16;
            if (bufferMB > 256) bufferMB = 256;
        }
        
        cout << Color::BRIGHT_CYAN << "  🎲 Random data? " << Color::BRIGHT_BLACK << "(y/n, default=n): " << Color::RESET;
        string randomInput;
        getline(cin, randomInput);
        randomData = (randomInput == "y" || randomInput == "Y");
    }
    
    if (threads == 0) {
        threads = max(4, static_cast<int>(thread::hardware_concurrency()));
    }
    
    long long fileSize = parseSizeInput(sizeInput);
    
    if (fileSize <= 0) {
        cout << Color::BRIGHT_RED << "\n  ❌ ERROR: Invalid size!\n" << Color::RESET;
        cout << Color::BRIGHT_YELLOW << "\n  📋 Examples:\n" << Color::RESET;
        cout << Color::WHITE << "    • 100 MB\n";
        cout << "    • 1 GB\n";
        cout << "    • 10 GB\n";
        cout << "    • 1073741824 (bytes)\n\n" << Color::RESET;
        return 1;
    }
    
    TurboFileCreator creator(fileName, fileSize, threads, bufferMB, randomData, turbo);
    
    if (creator.execute()) {
        cout << Color::BRIGHT_GREEN << Color::BOLD << "  🎉 SUCCESS! File created at maximum speed!\n\n" << Color::RESET;
        return 0;
    } else {
        cout << Color::BRIGHT_RED << Color::BOLD << "  💥 FAILED! Operation could not be completed.\n\n" << Color::RESET;
        return 1;
    }
}
