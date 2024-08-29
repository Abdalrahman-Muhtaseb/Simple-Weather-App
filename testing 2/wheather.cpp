#include <iostream>                       // Include standard input/output stream library
#include <thread>                         // Include thread library for multithreading
#include <atomic>                         // Include atomic library for atomic operations
#include <vector>                         // Include vector library for dynamic array
#include <unordered_map>                  // Include unordered_map library for hash table
#include <deque>                          // Include deque library for double-ended queue
#include "httplib.h"                      // Include httplib for HTTP requests
#include <json.hpp>                       // Include JSON library for parsing JSON data
#include <imgui.h>                        // Include ImGui library for GUI
#include <GLFW/glfw3.h>                   // Include GLFW library for OpenGL context and input handling
#include <imgui_impl_glfw.h>              // Include ImGui GLFW implementation
#include <imgui_impl_opengl3.h>           // Include ImGui OpenGL implementation
#include <chrono>                         // Include chrono library for time operations

// Define a structure to hold weather data
struct WeatherData {
    std::string city;                     // Name of the city
    double temperature;                   // Temperature in Celsius
    int humidity;                         // Humidity percentage
    double wind_speed;                    // Wind speed in m/s
};

// Function to fetch weather data from the OpenWeatherMap API
WeatherData fetchWeatherData(const std::string& city, const std::string& apiKey) {
    WeatherData weather;                  // Create a WeatherData object to store data
    weather.city = city;                  // Set the city name

    httplib::Client cli("http://api.openweathermap.org");  // Create an HTTP client
    std::string url = "/data/2.5/weather?q=" + city + "&appid=" + apiKey + "&units=metric"; // Construct the API request URL

    auto res = cli.Get(url.c_str());      // Send GET request to the API
    if (res && res->status == 200) {      // Check if request was successful
        auto json_data = nlohmann::json::parse(res->body);  // Parse JSON response
        weather.temperature = json_data["main"]["temp"];    // Extract temperature
        weather.humidity = json_data["main"]["humidity"];   // Extract humidity
        weather.wind_speed = json_data["wind"]["speed"];    // Extract wind speed
    }
    else {
        std::cerr << "Failed to fetch weather data for " << city << std::endl;  // Error message if request fails
    }

    return weather;                       // Return the fetched weather data
}

// Function to initialize and configure ImGui
void setupImGui(GLFWwindow* window) {
    IMGUI_CHECKVERSION();                        // Check ImGui version
    ImGui::CreateContext();                      // Create ImGui context
    ImGui::StyleColorsClassic();                 // Set ImGui to light theme
    ImGui_ImplGlfw_InitForOpenGL(window, true);  // Initialize ImGui for GLFW
    ImGui_ImplOpenGL3_Init("#version 130");      // Initialize ImGui for OpenGL with specific GLSL version
}

// Main function to run the application
int main() {
    std::string apiKey = "b07ff606d46c7139a6e5a10cbbf478f6";                 // OpenWeatherMap API key
    std::vector<std::string> cities = { "Jerusalem", "London", "New York" }; // List of suggested cities to fetch weather for
    std::unordered_map<std::string, WeatherData> weatherDataMap;             // Map to store weather data by city
    std::deque<std::string> favoriteCities;                                  // Deque to store favorite cities
    std::atomic<bool> dataFetched = false;                                   // Atomic flag to check if data has been fetched
    std::atomic<bool> keepRunning = true;                                    // Atomic flag to keep the weather thread running
    char citySearch[64] = "";                                                // Buffer to hold city name input for search
    WeatherData searchedCity;                                                // Variable to store searched city's weather data
    bool cityFound = false;                                                  // Flag to check if a city was found in the search

    // Create a thread to fetch weather data continuously
    std::thread weatherThread([&]() {
        while (keepRunning) {                      // Run while the flag is set to true
            for (const auto& city : cities) {
                weatherDataMap[city] = fetchWeatherData(city, apiKey); // Fetch and store weather data for each city
            }
            dataFetched = true;                    // Set flag to true after data is fetched
            for (int i = 0; i < 300; ++i) {        // Sleep in smaller intervals to check for shutdown
                if (!keepRunning) break;           // Check if the thread should stop
                std::this_thread::sleep_for(std::chrono::milliseconds(1000)); // Sleep for 1 second
            }
        }
        });

    // Setup GLFW and OpenGL
    if (!glfwInit()) {                      // Initialize GLFW
        std::cerr << "Failed to initialize GLFW" << std::endl;  // Error message if GLFW initialization fails
        return -1;
    }

    // Create a non-resizable window
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE); // Set window to be non-resizable
    GLFWwindow* window = glfwCreateWindow(700, 540, "Weather Application", nullptr, nullptr); // Create window
    if (!window) {                           // Check if window creation was successful
        glfwTerminate();                     // Terminate GLFW if window creation fails
        std::cerr << "Failed to create GLFW window" << std::endl;  // Error message if window creation fails
        return -1;
    }
    glfwMakeContextCurrent(window);          // Make the OpenGL context current
    setupImGui(window);                      // Setup ImGui with the created window

    // Main application loop
    while (!glfwWindowShouldClose(window)) { // Loop until the user closes the window
        glfwPollEvents();                    // Poll for events
        ImGui_ImplOpenGL3_NewFrame();        // Start new ImGui frame for OpenGL
        ImGui_ImplGlfw_NewFrame();           // Start new ImGui frame for GLFW
        ImGui::NewFrame();                   // Create a new ImGui frame

        // Search Window
        ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_Always); // Set position of the search window
        ImGui::SetNextWindowSize(ImVec2(360, 80), ImGuiCond_Always); // Set size of the search window
        ImGui::Begin("Search City", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove); // Create the search window
        ImGui::InputText("##CitySearch", citySearch, IM_ARRAYSIZE(citySearch)); // Input field for city search
        ImGui::SameLine();                  // Place the next widget on the same line
        if (ImGui::Button("Search")) {      // Button to trigger search
            std::string searchCity(citySearch); // Get the city name from input field
            if (!searchCity.empty()) {
                searchedCity = fetchWeatherData(searchCity, apiKey); // Fetch weather data for the searched city
                cityFound = true;            // Set flag if a city was found
            }
        }
        ImGui::End();                       // End the search window

        // Searched City Information Window
        ImGui::SetNextWindowPos(ImVec2(10, 100), ImGuiCond_Always); // Set position for searched city info window
        ImGui::SetNextWindowSize(ImVec2(360, 150), ImGuiCond_Always); // Set size for searched city info window
        ImGui::Begin("Searched City Information", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove); // Create the window
        if (cityFound) {                    // Display information if a city was found
            ImGui::Text("City: %s", searchedCity.city.c_str()); // Display city name
            ImGui::Text("Temperature: %.2f C", searchedCity.temperature); // Display temperature
            ImGui::Text("Humidity: %d%%", searchedCity.humidity); // Display humidity
            ImGui::Text("Wind Speed: %.2f m/s", searchedCity.wind_speed); // Display wind speed
            if (ImGui::Button("Add to Favorites")) { // Button to add city to favorites
                if (favoriteCities.size() >= 5) {
                    favoriteCities.pop_back(); // Remove oldest city if limit reached
                }
                favoriteCities.push_front(searchedCity.city); // Add city to the front of deque
                weatherDataMap[searchedCity.city] = searchedCity; // Store the weather data
            }
        }
        else {
            ImGui::Text("No city searched yet."); // Message if no city has been searched
        }
        ImGui::End();                       // End the searched city info window

        // Weather Information Window
        ImGui::SetNextWindowPos(ImVec2(10, 260), ImGuiCond_Always); // Set position for weather info window
        ImGui::SetNextWindowSize(ImVec2(360, 260), ImGuiCond_Always); // Set size for weather info window
        ImGui::Begin("Suggested Forcasts", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove); // Create the window
        if (dataFetched) {                  // Display data if it has been fetched
            for (const auto& city : cities) {
                const auto& weather = weatherDataMap[city]; // Get weather data for each city
                ImGui::Text("City: %s", weather.city.c_str()); // Display city name
                ImGui::Text("Temperature: %.2f C", weather.temperature); // Display temperature
                ImGui::Text("Humidity: %d%%", weather.humidity); // Display humidity
                ImGui::Text("Wind Speed: %.2f m/s", weather.wind_speed); // Display wind speed
                ImGui::Separator();         // Separator between cities
            }
        }
        else {
            ImGui::Text("Fetching weather data..."); // Message if data is still being fetched
        }
        ImGui::End();                       // End the weather info window

        // Favorite Cities Window
        ImGui::SetNextWindowPos(ImVec2(380, 10), ImGuiCond_Always); // Set position for favorites window
        ImGui::SetNextWindowSize(ImVec2(300, 510), ImGuiCond_Always); // Set size for favorites window
        ImGui::Begin("Favorite Cities", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove); // Create the window
        if (!favoriteCities.empty()) {      // Display favorite cities if any
            for (const auto& favorite : favoriteCities) {
                const auto& weather = weatherDataMap[favorite]; // Get weather data for each favorite city
                ImGui::Text("City: %s", weather.city.c_str()); // Display city name
                ImGui::Text("Temperature: %.2f C", weather.temperature); // Display temperature
                ImGui::Text("Humidity: %d%%", weather.humidity); // Display humidity
                ImGui::Text("Wind Speed: %.2f m/s", weather.wind_speed); // Display wind speed
                if (ImGui::Button(("Remove from Favorites##" + favorite).c_str())) { // Button to remove from favorites
                    favoriteCities.erase(std::remove(favoriteCities.begin(), favoriteCities.end(), favorite), favoriteCities.end());
                }
                ImGui::Separator();         // Separator between cities
            }
        }
        else {
            ImGui::Text("No favorite cities added."); // Message if no favorite cities added
        }
        ImGui::End();                       // End the favorites window

        ImGui::Render();                    // Render ImGui
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h); // Get framebuffer size
        glViewport(0, 0, display_w, display_h); // Set viewport
        glClearColor(0.45f, 0.55f, 0.60f, 1.00f); // Set clear color
        glClear(GL_COLOR_BUFFER_BIT);       // Clear the screen
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData()); // Render ImGui draw data
        glfwSwapBuffers(window);            // Swap buffers
    }

    // Cleanup
    keepRunning = false;                    // Set the flag to false to stop the weather fetching thread
    if (weatherThread.joinable()) {         // Join the weather fetching thread if joinable
        weatherThread.join();
    }

    ImGui_ImplOpenGL3_Shutdown();           // Shutdown ImGui OpenGL implementation
    ImGui_ImplGlfw_Shutdown();              // Shutdown ImGui GLFW implementation
    ImGui::DestroyContext();                // Destroy ImGui context
    glfwDestroyWindow(window);              // Destroy GLFW window
    glfwTerminate();                        // Terminate GLFW

    return 0;                               // Exit the program
}