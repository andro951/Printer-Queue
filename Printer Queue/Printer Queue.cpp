/*
Author - Isaac Richards
Date - 26SEP23
Description - Program used to simulate jobs being prioritized to a group of printers based on the printer with the least number of pages left to print.
    The Simulation settings, NUMBER_OF_PRINTERS, SIMULATION_SPEED, and SECONDS_TO_SIMULATE can be changed to alter the simulation.
*/

#include <iostream>
#include <queue>
#include <string>
#include <chrono>
#include <sstream>
#include <thread>
#include <random>
#include <vector>
#include <iomanip>

enum JobSizeID {
    Small,
    Medium,
    Large,
    VeryLarge
};

//Constants
const int MILLISECONDS_PER_SECOND = 1000;
const int SECONDS_PER_MINUTE = 60;
const int MINUTES_PER_HOUR = 60;
const int HOURS_PER_DAY = 24;
const int MILLISECONDS_PER_DAY = MILLISECONDS_PER_SECOND * SECONDS_PER_MINUTE * MINUTES_PER_HOUR * HOURS_PER_DAY;
const int SHEETS_PER_MINUTE = 7;
const int MILLISECONDS_PER_SHEET = MILLISECONDS_PER_SECOND * SECONDS_PER_MINUTE / SHEETS_PER_MINUTE;

//Simulation Settings
const int NUMBER_OF_PRINTERS = 4;
const int SIMULATION_SPEED = 300;//Simulated seconds per real second
const int SECONDS_TO_SIMULATE = SECONDS_PER_MINUTE * 30;//Run for 30 simulated minutes

/// <summary>
/// Tracks the simulated time of the simulation.
/// </summary>
static std::chrono::high_resolution_clock::time_point simulatedTime;

/// <summary>
/// Tracks the real time.  Used to update the simulated time.
/// </summary>
static std::chrono::high_resolution_clock::time_point realTime;

/// <returns>The simulatedTime as a string in HH:MM:SS format.</returns>
static std::string GetTime() {
    auto duration = simulatedTime.time_since_epoch();
    auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
    long long todayTime = milliseconds % MILLISECONDS_PER_DAY;
    int hours = todayTime / (MILLISECONDS_PER_SECOND * SECONDS_PER_MINUTE * MINUTES_PER_HOUR);
    todayTime -= hours * (MILLISECONDS_PER_SECOND * SECONDS_PER_MINUTE * MINUTES_PER_HOUR);
    int minutes = todayTime / (MILLISECONDS_PER_SECOND * SECONDS_PER_MINUTE);
    todayTime -= minutes * (MILLISECONDS_PER_SECOND * SECONDS_PER_MINUTE);
    int seconds = todayTime / MILLISECONDS_PER_SECOND;
    std::stringstream ss;
    ss << std::setw(2) << std::setfill('0') << hours << ":" << std::setw(2) << std::setfill('0') << minutes << ":" << std::setw(2) << std::setfill('0') << seconds;

    return ss.str();
}

/// <summary>
/// Convert number of pages to a JobSizeID.
/// </summary>
int GetJobSize(int pages) {
    if (pages <= 10)
        return Small;

    if (pages <= 25)
        return Medium;

    if (pages <= 50)
        return Large;

    return VeryLarge;
}

struct PrintJob {
    int ID;
    int Pages;
    PrintJob(int pages) {
        static int jobCount = 0;
        ID = jobCount++;
        Pages = pages;
    }

    int Size() {
        return GetJobSize(Pages);
    }

    std::string ToString() {
		std::stringstream ss;
		ss << "Job " << ID << " (" << Pages << " Pages)";
		return ss.str();
	}
};

class Printer {
    std::chrono::high_resolution_clock::time_point start;
    int pagesPrinted = 0;
    int totalPagesRemaining = 0;
    int printerID;
    std::queue<PrintJob> printQueue;
    bool printing = false;
public:
    Printer() {
        static int printerCount = 0;
        printerID = printerCount++;
    }

    void Update() {
        if (NoJobs())
            return;

        auto timeSinceStart = std::chrono::duration_cast<std::chrono::milliseconds>(simulatedTime - start);
        pagesPrinted = timeSinceStart.count() / MILLISECONDS_PER_SHEET;
        PrintJob& currentJob = printQueue.front();
        if (pagesPrinted > currentJob.Pages)
            pagesPrinted = currentJob.Pages;

        if (JobComplete()) {
            std::cout << GetTime() << " " << Name() << " finished printing " << currentJob.ToString() << std::endl;
            totalPagesRemaining -= currentJob.Pages;
            printQueue.pop();
            printing = false;
            CheckStartNextJob();

            std::cout << std::endl;
        }
    }

    int PagesLeft() {
        if (NoJobs())
            return 0;

        return printQueue.front().Pages - pagesPrinted;
    }

    bool JobComplete() {
        return PagesLeft() == 0;
    }

    bool IsIdle() {
        return !printing;
    }

    bool NoJobs() {
        return printQueue.size() < 1;
    }

    void Print(PrintJob job) {
        printQueue.push(job);
        std::cout << GetTime() << " " << Name() << " added job to the queue " << job.ToString() << std::endl;
        totalPagesRemaining += job.Pages;
        if (printQueue.size() == 1)
            CheckStartNextJob();
    }

    void CheckStartNextJob() {
        if (NoJobs())
            return;

        if (!IsIdle())
            return;

        PrintJob currentJob = printQueue.front();
        start = simulatedTime;
        printing = true;
        std::cout << GetTime() << " " << Name() << " started printing " << currentJob.ToString() << std::endl;
    }

    int GetTotalPagesLeft() {
        if (NoJobs())
            return 0;

        return totalPagesRemaining - pagesPrinted;
    }

    std::string Name() {
        std::stringstream ss;
        ss << "Printer " << printerID;
        return ss.str();
    }

    void LogRemainingJobs() {
        if (NoJobs()) {
            std::cout << "No jobs remaining." << std::endl;
			return;
        }

        bool first = true;
        while (printQueue.size() > 0) {
            PrintJob job = printQueue.front();
            if (first) {
				first = false;
                std::cout << "Job " << job.ID << " (" << job.Pages << " Pages, " << job.Pages - pagesPrinted << " Remaining)";
			}
            else {
                std::cout << ", " << job.ToString();
			}

			printQueue.pop();
        }
    }
};

static std::vector<Printer> printers;

int GetRandomPrintJob() {
    int r = rand() % 10;
    if (r <= 3)
        return rand() % 10 + 1;

    if (r <= 6)
        return rand() % 15 + 11;

    if (r <= 8)
        return rand() % 25 + 26;

    return rand() % 49 + 51;
}

void Update() {
    //std::cout << "Update " << GetTime() << std::endl;
    for (int i = 0; i < printers.size(); i++) {
        Printer& printer = printers[i];
        printer.Update();
    }

    static auto nextJobTime = simulatedTime;
    static auto timePerJob = std::chrono::seconds(30);
    auto timeSinceLastJob = std::chrono::duration_cast<std::chrono::seconds>(simulatedTime - nextJobTime);
    //Add a new job every 30 seconds
    if (timeSinceLastJob >= timePerJob) {
        nextJobTime += timePerJob;
        int jobSize = GetRandomPrintJob();
        PrintJob job(jobSize);
        std::cout << GetTime() << " created " << job.ToString() << std::endl;
        //Find the printer with the least amount of pages left to print and add the job to that printer
        int selectedForNewJob = 0;
        for (int i = 0; i < printers.size(); i++) {
            Printer& printer = printers[i];
            if (printer.NoJobs()) {
                selectedForNewJob = i;
                break;
            }

            if (i == 0)
                continue;

            Printer& selectedPrinter = printers[selectedForNewJob];
            int selectedPrinterPagesLeft = selectedPrinter.GetTotalPagesLeft();
            int printerPagesLeft = printer.GetTotalPagesLeft();
            if (printerPagesLeft < selectedPrinterPagesLeft)
                selectedForNewJob = i;
        }

        Printer& selectedPrinter = printers[selectedForNewJob];
        selectedPrinter.Print(job);
        std::cout << std::endl;
    }
}

bool ShouldUpdate() {
    static auto lastUpdate = std::chrono::high_resolution_clock::now();
    long long timeSinceLastUpdate = std::chrono::duration_cast<std::chrono::milliseconds>(simulatedTime - lastUpdate).count();
    //Update simulation once per simulated second.
    if (timeSinceLastUpdate >= MILLISECONDS_PER_SECOND) {
        lastUpdate += std::chrono::milliseconds(MILLISECONDS_PER_SECOND);
        return true;
    }

    return false;
}

void Setup() {
    //seed random number generator with current time
    std::srand(static_cast<unsigned int>(std::time(nullptr)));

    //Create printers
    for (int i = 0; i < NUMBER_OF_PRINTERS; i++) {
        printers.push_back(Printer());
    }

    simulatedTime = std::chrono::high_resolution_clock::now();
    realTime = simulatedTime;
}

void Run() {
    std::chrono::high_resolution_clock::time_point end = simulatedTime + std::chrono::seconds(SECONDS_TO_SIMULATE);
    while (simulatedTime < end) {
        //Update the simulated time
        std::chrono::high_resolution_clock::time_point now = std::chrono::high_resolution_clock::now();
        long long timeSinceLastUpdate = std::chrono::duration_cast<std::chrono::microseconds>(now - realTime).count();
        realTime += std::chrono::microseconds(timeSinceLastUpdate);
        //Increase the simulated time by the amount of time that has passed since the last update multiplied by the simulation speed
        simulatedTime += std::chrono::microseconds(timeSinceLastUpdate * SIMULATION_SPEED);
        if (ShouldUpdate())
            Update();

        //Sleep
        std::this_thread::sleep_for(std::chrono::microseconds(1));
    }
}

void Cleanup() {
    //Print the final status of the printers
    std::cout << "\nSimulation ended at " << GetTime() << ".\nStatus of Printers:\n";
    for (int i = 0; i < printers.size(); i++) {
        Printer& printer = printers[i];
        std::cout << printer.Name();
        std::cout << " - Total pages left: " << printer.GetTotalPagesLeft() << ", ";
        printer.LogRemainingJobs();
        std::cout << std::endl;
    }
}

int main() {
    Setup();
	Run();
	Cleanup();
	return 0;
}