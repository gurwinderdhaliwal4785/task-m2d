#include <iostream>
#include <fstream>
#include <thread>
#include <mutex>
#include <queue>
#include <chrono>
#include <ctime>
#include <sstream>
#include <map>

using namespace std;

const int NUM_TRAFFIC_LIGHTS = 5;
const int BUFFER_SIZE = 10;
const int NUM_MEASUREMENTS = 12;
const int MEASUREMENT_INTERVAL_SECONDS = 60; // 1 minute in seconds
const int TOP_N_CONGESTED = 5;

mutex buffer_lock;
queue<tuple<string, int, int>> traffic_buffer;
map<int, int> congested_traffic_lights;

// Producer function
void produce_traffic_data() {
    ifstream input_file("input.txt");
    if (!input_file.is_open()) {
        cerr << "Error opening input file." << endl;
        return;
    }

    string line;
    while (getline(input_file, line)) {
        istringstream iss(line);
        string date, time;
        int traffic_light_id, cars_passed;
        iss >> date >> time >> traffic_light_id >> cars_passed;
        string timestamp = date + " " + time;

        // Add data to buffer
        {
            lock_guard<mutex> guard(buffer_lock);
            if (traffic_buffer.size() >= BUFFER_SIZE) {
                cout << "Buffer full, waiting to add data..." << endl;
            }
            traffic_buffer.push(make_tuple(timestamp, traffic_light_id, cars_passed));
            cout << "Produced: " << timestamp << ", Traffic Light ID: " << traffic_light_id << ", Cars Passed: " << cars_passed << endl;
        }
        this_thread::sleep_for(chrono::milliseconds(rand() % 1000 + 100)); // Simulate variable time to produce data
    }
    input_file.close();
}


// Consumer function
void consume_traffic_data() {
    ofstream output_file("output.txt", ios::app); // Open output file in append mode
    if (!output_file.is_open()) {
        cerr << "Error opening output file." << endl;
        return;
    }

    while (true) {
        this_thread::sleep_for(chrono::seconds(MEASUREMENT_INTERVAL_SECONDS));

        // Process traffic data
        {
            lock_guard<mutex> guard(buffer_lock);
            while (!traffic_buffer.empty()) {
                auto data = traffic_buffer.front();
                traffic_buffer.pop();
                string timestamp = get<0>(data);
                int traffic_light_id = get<1>(data);
                int cars_passed = get<2>(data);
                if (congested_traffic_lights.find(traffic_light_id) != congested_traffic_lights.end()) {
                    congested_traffic_lights[traffic_light_id] += cars_passed;
                } else {
                    congested_traffic_lights[traffic_light_id] = cars_passed;
                }
            }
        }

        // Find top N congested traffic lights
        vector<pair<int, int>> sorted_congested(congested_traffic_lights.begin(), congested_traffic_lights.end());
        partial_sort(sorted_congested.begin(), sorted_congested.begin() + min(TOP_N_CONGESTED, (int)sorted_congested.size()), sorted_congested.end(),
                     [](const pair<int, int> &a, const pair<int, int> &b) { return a.second > b.second; });

        // Write to output file
        time_t now_time = chrono::system_clock::to_time_t(chrono::system_clock::now());
        tm* now_tm = localtime(&now_time);
        char buffer[80];
        strftime(buffer, 80, "Hour %d-%m-%Y on %H:%M:%S", now_tm);
        output_file << buffer << endl;
        output_file << "Top " << min(TOP_N_CONGESTED, (int)sorted_congested.size()) << " Congested Traffic Lights:" << endl;
        output_file << "**********************************" << endl;
        for (const auto& pair : sorted_congested) {
            output_file << "Traffic Light " << pair.first << ":" << endl;
            output_file << pair.second << " cars" << endl;
        }
        output_file << "****************************" << endl << endl;

        // Clear the congestion data for the next interval
        congested_traffic_lights.clear();
    }
}



int main() {
    // Create producer and consumer threads
    thread producer_thread(produce_traffic_data);
    thread consumer_thread(consume_traffic_data);

    // Join threads
    producer_thread.join();
    consumer_thread.join();

    return 0;
}
