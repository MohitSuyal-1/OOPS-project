#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <SDL_opengl.h>
#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_opengl3.h"
#include "datamanager.h"

#include <vector>
#include <string>

DataManager g_dataManager;

int g_currentPage = 1; // 1-7

// Buffers for inputs
char g_fromBuf[256] = "";
char g_toBuf[256] = "";
char g_nameSearchBuf[256] = "";
char g_trainNoAvailBuf[256] = "";
char g_passengerNameBuf[256] = "";
char g_ageBuf[10] = "18";
char g_pnrViewBuf[256] = "";
char g_pnrCancelBuf[256] = "";

// For selections
int g_selectedTrainIdx = -1;
int g_selectedClassIdx = -1;
std::vector<std::string> g_trainList;
std::vector<std::string> g_classList;

// Results
std::vector<Train> g_searchResults;
std::vector<Train> g_availResults; // for availability table
std::vector<Booking> g_bookingResults;

// Messages
std::string g_message = "";
ImVec4 g_msgColor = ImVec4(1, 1, 1, 1);
bool g_showMsgPopup = false;

// Forward declarations
void showMessagePopup(const std::string& msg, const ImVec4& color);
void SetupImGuiStyle();

// Populate helpers
void PopulateTrainList() {
    g_trainList.clear();
    for (const auto& t : g_dataManager.trains()) {
        g_trainList.push_back(t.trainNo + " - " + t.trainName);
    }
}

void UpdateClassListForTrain(int trainIdx) {
    if (trainIdx < 0) return;
    g_classList.clear();
    auto& classes = g_dataManager.trains()[trainIdx].classes;
    for (const auto& c : classes) {
        g_classList.push_back(c);
    }
    g_selectedClassIdx = -1;
}

int main(int argc, char* argv[]) {
    // Init SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
        printf("Error: %s\n", SDL_GetError());
        return -1;
    }

    // GL attributes
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

    // Create window (fullscreen)
    SDL_DisplayMode current;
    SDL_GetCurrentDisplayMode(0, &current);
    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_FULLSCREEN);
    SDL_Window* window = SDL_CreateWindow("Ticket Reservation System",
                                          SDL_WINDOWPOS_CENTERED,
                                          SDL_WINDOWPOS_CENTERED,
                                          current.w,
                                          current.h,
                                          window_flags);

    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, gl_context);
    SDL_GL_SetSwapInterval(1); // vsync

    // Init ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    // Increase global font size a bit
    io.FontGlobalScale = 1.2f;

    // Custom theme (colors, rounding, etc.)
    SetupImGuiStyle();

    ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init("#version 130");

    // Load data
    g_dataManager.setTrainFilePath("train.csv");
    g_dataManager.setBookingFilePath("booking.csv");
    g_dataManager.loadTrains();
    g_dataManager.loadBookings();
    PopulateTrainList();

    // Main loop
    bool done = false;
    while (!done) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT)
                done = true;
        }

        // Start ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        // Full screen main window
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->Pos);
        ImGui::SetNextWindowSize(viewport->Size);
        ImGui::Begin("Main",
                     nullptr,
                     ImGuiWindowFlags_NoTitleBar
                     | ImGuiWindowFlags_NoCollapse
                     | ImGuiWindowFlags_NoResize
                     | ImGuiWindowFlags_NoMove
                     | ImGuiWindowFlags_NoBringToFrontOnFocus);

        // ======= TOP HEADER BAR =======
        {
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 10));
            ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.07f, 0.09f, 0.18f, 1.0f));

            ImGui::BeginChild("HeaderBar", ImVec2(0, 70), true);

            ImGui::TextColored(ImVec4(0.3f, 0.8f, 1.0f, 1.0f),
                               "Ticket Reservation System");
            ImGui::Spacing();
            ImGui::TextDisabled("Smart Reservation Panel");

            ImGui::EndChild();
            ImGui::PopStyleColor();
            ImGui::PopStyleVar();
        }

        ImGui::Spacing();

        // ======= MAIN BODY (Sidebar + Content) =======
        float sidebarWidth = 260.0f;
        float footerHeight = ImGui::GetFrameHeightWithSpacing() + 6.0f;

        // Sidebar
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.08f, 0.10f, 0.18f, 1.0f));
        ImGui::BeginChild("Sidebar",
                          ImVec2(sidebarWidth, viewport->Size.y - 80.0f - footerHeight),
                          true);

        ImGui::SeparatorText("  MENU  ");

        ImGui::Spacing();
        auto menuButton = [](const char* label, int page) {
            bool active = (g_currentPage == page);
            if (active) {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.20f, 0.36f, 0.72f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.25f, 0.45f, 0.85f, 1.0f));
            }
            if (ImGui::Button(label, ImVec2(-1, 0))) {
                g_currentPage = page;
            }
            if (active) {
                ImGui::PopStyleColor(2);
            }
        };

        menuButton("All Trains",        1);
        menuButton("Search Route",      2);
        menuButton("Search by Name",    3);
        menuButton("Seat Availability", 4);
        menuButton("Book Ticket",       5);
        menuButton("View Ticket",       6);
        menuButton("Cancel Ticket",     7);

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::TextDisabled("Passenger Safety Tips:");
        ImGui::BulletText("Keep your luggage with you at all times.");
        ImGui::BulletText("Do not share your PNR or ID with strangers.");
        ImGui::BulletText("Verify coach and seat number before boarding.");

        ImGui::EndChild();
        ImGui::PopStyleColor();

        ImGui::SameLine();

        // Content area
        ImGui::BeginChild("Content",
                          ImVec2(0, viewport->Size.y - 80.0f - footerHeight),
                          true);

        switch (g_currentPage) {
            case 1: {
                ImGui::SeparatorText("  ALL TRAINS  ");

                if (ImGui::Button("Reload Trains from CSV")) {
                    if (g_dataManager.loadTrains()) {
                        PopulateTrainList();
                        showMessagePopup("Train list reloaded from CSV.", ImVec4(0.3f, 1.0f, 0.3f, 1.0f));
                    } else {
                        showMessagePopup("Failed to load train.csv", ImVec4(1.0f, 0.3f, 0.3f, 1.0f));
                    }
                }

                ImGui::Spacing();

                static ImGuiTableFlags flags =
                    ImGuiTableFlags_Borders
                    | ImGuiTableFlags_RowBg
                    | ImGuiTableFlags_Resizable
                    | ImGuiTableFlags_SizingStretchProp;

                if (ImGui::BeginTable("TrainsTable", 7, flags)) {
                    ImGui::TableSetupColumn("Train No", ImGuiTableColumnFlags_WidthFixed, 80.0f);
                    ImGui::TableSetupColumn("Train Name");
                    ImGui::TableSetupColumn("From", ImGuiTableColumnFlags_WidthFixed, 100.0f);
                    ImGui::TableSetupColumn("To", ImGuiTableColumnFlags_WidthFixed, 100.0f);
                    ImGui::TableSetupColumn("Arrival", ImGuiTableColumnFlags_WidthFixed, 100.0f);
                    ImGui::TableSetupColumn("Departure", ImGuiTableColumnFlags_WidthFixed, 100.0f);
                    ImGui::TableSetupColumn("Classes");
                    ImGui::TableHeadersRow();

                    for (const auto& train : g_dataManager.trains()) {
                        ImGui::TableNextRow();
                        ImGui::TableNextColumn(); ImGui::Text("%s", train.trainNo.c_str());
                        ImGui::TableNextColumn(); ImGui::Text("%s", train.trainName.c_str());
                        ImGui::TableNextColumn(); ImGui::Text("%s", train.from.c_str());
                        ImGui::TableNextColumn(); ImGui::Text("%s", train.to.c_str());
                        ImGui::TableNextColumn(); ImGui::Text("%s", train.arr.c_str());
                        ImGui::TableNextColumn(); ImGui::Text("%s", train.dep.c_str());

                        ImGui::TableNextColumn();
                        std::string classesStr;
                        for (const auto& c : train.classes) {
                            if (!classesStr.empty()) classesStr += " ";
                            classesStr += c;
                        }
                        ImGui::Text("%s", classesStr.c_str());
                    }
                    ImGui::EndTable();
                }
                break;
            }
            case 2: {
                ImGui::SeparatorText("  SEARCH BY ROUTE  ");
                ImGui::InputText("From (Station)", g_fromBuf, IM_ARRAYSIZE(g_fromBuf));
                ImGui::InputText("To (Station)",   g_toBuf,   IM_ARRAYSIZE(g_toBuf));
                if (ImGui::Button("Search Route")) {
                    g_searchResults = g_dataManager.searchTrainsByRoute(g_fromBuf, g_toBuf);
                    if (g_searchResults.empty()) {
                        showMessagePopup("No trains found for this route.", ImVec4(1, 0.5f, 0.2f, 1.0f));
                    }
                }

                ImGui::Spacing();

                if (ImGui::BeginTable("SearchTable", 7, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
                    ImGui::TableSetupColumn("Train No", ImGuiTableColumnFlags_WidthFixed, 80.0f);
                    ImGui::TableSetupColumn("Train Name");
                    ImGui::TableSetupColumn("From", ImGuiTableColumnFlags_WidthFixed, 100.0f);
                    ImGui::TableSetupColumn("To", ImGuiTableColumnFlags_WidthFixed, 100.0f);
                    ImGui::TableSetupColumn("Arrival");
                    ImGui::TableSetupColumn("Departure");
                    ImGui::TableSetupColumn("Classes");
                    ImGui::TableHeadersRow();

                    for (const auto& train : g_searchResults) {
                        ImGui::TableNextRow();
                        ImGui::TableNextColumn(); ImGui::Text("%s", train.trainNo.c_str());
                        ImGui::TableNextColumn(); ImGui::Text("%s", train.trainName.c_str());
                        ImGui::TableNextColumn(); ImGui::Text("%s", train.from.c_str());
                        ImGui::TableNextColumn(); ImGui::Text("%s", train.to.c_str());
                        ImGui::TableNextColumn(); ImGui::Text("%s", train.arr.c_str());
                        ImGui::TableNextColumn(); ImGui::Text("%s", train.dep.c_str());

                        ImGui::TableNextColumn();
                        std::string classesStr;
                        for (const auto& c : train.classes) {
                            if (!classesStr.empty()) classesStr += " ";
                            classesStr += c;
                        }
                        ImGui::Text("%s", classesStr.c_str());
                    }
                    ImGui::EndTable();
                }
                break;
            }
            case 3: {
                ImGui::SeparatorText("  SEARCH BY TRAIN NAME  ");
                ImGui::InputText("Train Name", g_nameSearchBuf, IM_ARRAYSIZE(g_nameSearchBuf));
                if (ImGui::Button("Search Name")) {
                    g_searchResults = g_dataManager.searchTrainsByName(g_nameSearchBuf);
                    if (g_searchResults.empty()) {
                        showMessagePopup("No train found with that name.", ImVec4(1, 0.5f, 0.2f, 1.0f));
                    }
                }

                ImGui::Spacing();

                if (ImGui::BeginTable("NameTable", 7, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
                    ImGui::TableSetupColumn("Train No", ImGuiTableColumnFlags_WidthFixed, 80.0f);
                    ImGui::TableSetupColumn("Train Name");
                    ImGui::TableSetupColumn("From");
                    ImGui::TableSetupColumn("To");
                    ImGui::TableSetupColumn("Arrival");
                    ImGui::TableSetupColumn("Departure");
                    ImGui::TableSetupColumn("Classes");
                    ImGui::TableHeadersRow();

                    for (const auto& train : g_searchResults) {
                        ImGui::TableNextRow();
                        ImGui::TableNextColumn(); ImGui::Text("%s", train.trainNo.c_str());
                        ImGui::TableNextColumn(); ImGui::Text("%s", train.trainName.c_str());
                        ImGui::TableNextColumn(); ImGui::Text("%s", train.from.c_str());
                        ImGui::TableNextColumn(); ImGui::Text("%s", train.to.c_str());
                        ImGui::TableNextColumn(); ImGui::Text("%s", train.arr.c_str());
                        ImGui::TableNextColumn(); ImGui::Text("%s", train.dep.c_str());

                        ImGui::TableNextColumn();
                        std::string classesStr;
                        for (const auto& c : train.classes) {
                            if (!classesStr.empty()) classesStr += " ";
                            classesStr += c;
                        }
                        ImGui::Text("%s", classesStr.c_str());
                    }
                    ImGui::EndTable();
                }
                break;
            }
            case 4: {
                ImGui::SeparatorText("  SEAT AVAILABILITY  ");
                ImGui::InputText("Train Number", g_trainNoAvailBuf, IM_ARRAYSIZE(g_trainNoAvailBuf));
                if (ImGui::Button("Check Availability")) {
                    const Train* t = g_dataManager.findTrainByNumber(g_trainNoAvailBuf);
                    if (t) {
                        g_availResults = { *t };
                        showMessagePopup("Availability loaded.", ImVec4(0.3f, 1.0f, 0.3f, 1.0f));
                    } else {
                        g_availResults.clear();
                        showMessagePopup("Train not found.", ImVec4(1, 0.3f, 0.3f, 1.0f));
                    }
                }

                ImGui::Spacing();

                if (!g_availResults.empty()) {
                    const Train& t = g_availResults[0];
                    ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f),
                                       "Train: %s - %s",
                                       t.trainNo.c_str(), t.trainName.c_str());
                    ImGui::Text("Route: %s -> %s", t.from.c_str(), t.to.c_str());
                    ImGui::Text("Departure: %s", t.dep.c_str());

                    ImGui::Spacing();

                    if (ImGui::BeginTable("AvailTable", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
                        ImGui::TableSetupColumn("Class");
                        ImGui::TableSetupColumn("Total Seats");
                        ImGui::TableSetupColumn("Booked");
                        ImGui::TableSetupColumn("Available");
                        ImGui::TableSetupColumn("Fare (Rs)");
                        ImGui::TableHeadersRow();

                        for (const auto& cls : t.classes) {
                            int total  = g_dataManager.seatCapacity().at(cls);
                            int booked = g_dataManager.bookedCount(t.trainNo, cls);
                            int avail  = total - booked;
                            int fare   = g_dataManager.fares().at(cls);

                            ImGui::TableNextRow();
                            ImGui::TableNextColumn(); ImGui::Text("%s", cls.c_str());
                            ImGui::TableNextColumn(); ImGui::Text("%d", total);
                            ImGui::TableNextColumn(); ImGui::Text("%d", booked);
                            ImGui::TableNextColumn(); ImGui::Text("%d", avail);
                            ImGui::TableNextColumn(); ImGui::Text("%d", fare);
                        }
                        ImGui::EndTable();
                    }
                }
                break;
            }
            case 5: {
                ImGui::SeparatorText("  BOOK TICKET  ");

                ImGui::InputText("Passenger Name", g_passengerNameBuf, IM_ARRAYSIZE(g_passengerNameBuf));
                ImGui::InputText("Age", g_ageBuf, IM_ARRAYSIZE(g_ageBuf));

                ImGui::Spacing();

                ImGui::Text("Select Train:");
                if (ImGui::BeginListBox("##trainlist", ImVec2(-FLT_MIN, 150))) {
                    for (int i = 0; i < (int)g_trainList.size(); i++) {
                        const bool is_selected = (g_selectedTrainIdx == i);
                        if (ImGui::Selectable(g_trainList[i].c_str(), is_selected)) {
                            g_selectedTrainIdx = i;
                            UpdateClassListForTrain(i);
                        }
                        if (is_selected) ImGui::SetItemDefaultFocus();
                    }
                    ImGui::EndListBox();
                }

                if (g_selectedTrainIdx >= 0) {
                    const Train& t = g_dataManager.trains()[g_selectedTrainIdx];
                    ImGui::TextColored(ImVec4(0.8f, 0.85f, 0.2f, 1.0f),
                                       "Train: %s - %s", t.trainNo.c_str(), t.trainName.c_str());
                    ImGui::Text("Route: %s -> %s | Departure: %s",
                                t.from.c_str(), t.to.c_str(), t.dep.c_str());
                }

                ImGui::Spacing();
                ImGui::Text("Select Class:");

                for (int i = 0; i < (int)g_classList.size(); i++) {
                    bool is_selected = (g_selectedClassIdx == i);
                    if (ImGui::RadioButton(g_classList[i].c_str(), is_selected)) {
                        g_selectedClassIdx = i;
                    }
                    if ((i + 1) % 4 != 0) ImGui::SameLine();
                }

                ImGui::Spacing();

                if (g_selectedClassIdx >= 0) {
                    int fare = g_dataManager.fares().at(g_classList[g_selectedClassIdx]);
                    ImGui::Text("Fare: Rs. %d", fare);
                    if (g_selectedTrainIdx >= 0) {
                        const Train& t = g_dataManager.trains()[g_selectedTrainIdx];
                        int avail = g_dataManager.seatCapacity().at(g_classList[g_selectedClassIdx])
                                    - g_dataManager.bookedCount(t.trainNo, g_classList[g_selectedClassIdx]);
                        ImGui::Text("Seats Left: %d", avail);
                    }
                }

                ImGui::Spacing();

                if (ImGui::Button("Confirm Booking", ImVec2(200, 0))) {
                    if (strlen(g_passengerNameBuf) == 0) {
                        showMessagePopup("Please enter passenger name.", ImVec4(1, 0.3f, 0.3f, 1.0f));
                    } else {
                        int age;
                        try {
                            age = std::stoi(g_ageBuf);
                            if (age < 1 || age > 120) throw std::exception();
                        } catch (...) {
                            showMessagePopup("Please enter a valid age (1-120).", ImVec4(1, 0.3f, 0.3f, 1.0f));
                            break;
                        }

                        if (g_selectedTrainIdx < 0 || g_selectedClassIdx < 0) {
                            showMessagePopup("Select train and class.", ImVec4(1, 0.3f, 0.3f, 1.0f));
                            break;
                        }

                        const Train& t = g_dataManager.trains()[g_selectedTrainIdx];
                        std::string cls = g_classList[g_selectedClassIdx];
                        int avail = g_dataManager.seatCapacity().at(cls)
                                    - g_dataManager.bookedCount(t.trainNo, cls);

                        if (avail <= 0) {
                            showMessagePopup("No seats available in this class.", ImVec4(1, 0.3f, 0.3f, 1.0f));
                            break;
                        }

                        Booking b;
                        b.pnr       = g_dataManager.generatePNR();
                        b.name      = g_passengerNameBuf;
                        b.age       = age;
                        b.trainNo   = t.trainNo;
                        b.trainName = t.trainName;
                        b.classType = cls;
                        b.seatNo    = g_dataManager.nextSeatNo(t.trainNo, cls);
                        b.fare      = g_dataManager.fares().at(cls);
                        b.departure = t.dep;

                        if (g_dataManager.addBooking(b)) {
                            std::string msg =
                                "Booking Confirmed!\n"
                                "PNR: " + b.pnr +
                                "\nPassenger: " + b.name +
                                "\nTrain: " + b.trainNo + " - " + b.trainName +
                                "\nUse 'View Ticket' page for full details.";

                            // Softer blue-green color instead of harsh green
                            showMessagePopup(msg, ImVec4(0.3f, 0.8f, 1.0f, 1.0f));

                            // Clear form
                            memset(g_passengerNameBuf, 0, sizeof(g_passengerNameBuf));
                            strcpy(g_ageBuf, "18");
                            g_selectedTrainIdx = -1;
                            g_selectedClassIdx = -1;
                            g_classList.clear();
                        } else {
                            showMessagePopup("Failed to save booking.", ImVec4(1, 0.3f, 0.3f, 1.0f));
                        }
                    }
                }
                break;
            }
            case 6: {
                ImGui::SeparatorText("  VIEW TICKET  ");
                ImGui::InputText("Enter PNR", g_pnrViewBuf, IM_ARRAYSIZE(g_pnrViewBuf));
                if (ImGui::Button("Search Ticket")) {
                    g_bookingResults = g_dataManager.findBookingsByPNR(g_pnrViewBuf);
                    if (g_bookingResults.empty()) {
                        showMessagePopup("No ticket found for this PNR.", ImVec4(1, 0.3f, 0.3f, 1.0f));
                    }
                }

                ImGui::Spacing();

                if (!g_bookingResults.empty()) {
                    const Booking& b = g_bookingResults[0];

                    ImGui::BeginChild("TicketCard", ImVec2(0, 0), true);
                    ImGui::TextColored(ImVec4(0.3f, 1.0f, 1.0f, 1.0f), "Ticket Details");
                    ImGui::Separator();
                    ImGui::Text("PNR: %s", b.pnr.c_str());
                    ImGui::Text("Name: %s (Age: %d)", b.name.c_str(), b.age);
                    ImGui::Text("Train: %s - %s", b.trainNo.c_str(), b.trainName.c_str());
                    ImGui::Text("Class: %s  Seat: %d", b.classType.c_str(), b.seatNo);
                    ImGui::Text("Fare: Rs. %d", b.fare);
                    ImGui::Text("Departure: %s", b.departure.c_str());
                    ImGui::EndChild();
                }
                break;
            }
            case 7: {
                ImGui::SeparatorText("  CANCEL TICKET  ");
                ImGui::InputText("Enter PNR", g_pnrCancelBuf, IM_ARRAYSIZE(g_pnrCancelBuf));
                if (ImGui::Button("Find Ticket")) {
                    g_bookingResults = g_dataManager.findBookingsByPNR(g_pnrCancelBuf);
                    if (g_bookingResults.empty()) {
                        showMessagePopup("No ticket found for this PNR.", ImVec4(1, 0.3f, 0.3f, 1.0f));
                    }
                }

                ImGui::Spacing();

                if (!g_bookingResults.empty()) {
                    const Booking& b = g_bookingResults[0];

                    ImGui::BeginChild("CancelCard", ImVec2(0, 0), true);
                    ImGui::TextColored(ImVec4(0.3f, 1.0f, 1.0f, 1.0f), "Ticket Found");
                    ImGui::Separator();
                    ImGui::Text("PNR: %s", b.pnr.c_str());
                    ImGui::Text("Name: %s (Age: %d)", b.name.c_str(), b.age);
                    ImGui::Text("Train: %s - %s", b.trainNo.c_str(), b.trainName.c_str());
                    ImGui::Text("Class: %s  Seat: %d", b.classType.c_str(), b.seatNo);
                    ImGui::Text("Fare: Rs. %d", b.fare);
                    ImGui::Text("Departure: %s", b.departure.c_str());
                    ImGui::Spacing();

                    if (ImGui::Button("Confirm Cancellation", ImVec2(220, 0))) {
                        Booking removed;
                        if (g_dataManager.cancelBooking(b.pnr, &removed)) {
                            showMessagePopup("Ticket cancelled successfully.", ImVec4(0.3f, 1.0f, 0.3f, 1.0f));
                            memset(g_pnrCancelBuf, 0, sizeof(g_pnrCancelBuf));
                            g_bookingResults.clear();
                        } else {
                            showMessagePopup("Failed to cancel ticket.", ImVec4(1, 0.3f, 0.3f, 1.0f));
                        }
                    }

                    ImGui::EndChild();
                }
                break;
            }
        }

        ImGui::EndChild(); // Content

        // Footer
        ImGui::SetCursorPosY(viewport->Size.y - footerHeight);
        ImGui::Separator();
        ImGui::TextDisabled("Smart Reservation Panel  |  Please keep your luggage safe during travel.");

        ImGui::End(); // Main window

        // Message popup
        if (g_showMsgPopup) {
            ImGui::OpenPopup("Message");
            g_showMsgPopup = false;
        }
        if (ImGui::BeginPopupModal("Message", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::TextColored(g_msgColor, "%s", g_message.c_str());
            ImGui::Spacing();
            if (ImGui::Button("OK", ImVec2(120, 0))) {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        // Rendering
        ImGui::Render();
        glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
        glClearColor(0.05f, 0.06f, 0.10f, 1.0f); // darker, bluish theme
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(window);
    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}

void showMessagePopup(const std::string& msg, const ImVec4& color) {
    g_message = msg;
    g_msgColor = color;
    g_showMsgPopup = true;
}

void SetupImGuiStyle() {
    ImGuiStyle& style = ImGui::GetStyle();

    style.WindowRounding = 8.0f;
    style.ChildRounding  = 8.0f;
    style.FrameRounding  = 6.0f;
    style.ScrollbarRounding = 6.0f;
    style.GrabRounding   = 6.0f;
    style.TabRounding    = 6.0f;

    style.WindowBorderSize = 0.0f;
    style.FrameBorderSize  = 0.0f;

    ImVec4* colors = style.Colors;
    colors[ImGuiCol_WindowBg]            = ImVec4(0.06f, 0.07f, 0.12f, 1.0f);
    colors[ImGuiCol_ChildBg]             = ImVec4(0.07f, 0.08f, 0.13f, 1.0f);
    colors[ImGuiCol_Header]              = ImVec4(0.20f, 0.36f, 0.72f, 0.75f);
    colors[ImGuiCol_HeaderHovered]       = ImVec4(0.25f, 0.45f, 0.85f, 0.86f);
    colors[ImGuiCol_HeaderActive]        = ImVec4(0.30f, 0.52f, 0.90f, 1.00f);

    colors[ImGuiCol_Button]              = ImVec4(0.18f, 0.30f, 0.55f, 1.00f);
    colors[ImGuiCol_ButtonHovered]       = ImVec4(0.24f, 0.40f, 0.75f, 1.00f);
    colors[ImGuiCol_ButtonActive]        = ImVec4(0.30f, 0.50f, 0.90f, 1.00f);

    colors[ImGuiCol_FrameBg]             = ImVec4(0.10f, 0.12f, 0.18f, 1.00f);
    colors[ImGuiCol_FrameBgHovered]      = ImVec4(0.18f, 0.30f, 0.55f, 1.00f);
    colors[ImGuiCol_FrameBgActive]       = ImVec4(0.22f, 0.38f, 0.70f, 1.00f);

    colors[ImGuiCol_TitleBg]             = ImVec4(0.05f, 0.06f, 0.10f, 1.00f);
    colors[ImGuiCol_TitleBgActive]       = ImVec4(0.10f, 0.12f, 0.18f, 1.00f);

    colors[ImGuiCol_Tab]                 = ImVec4(0.12f, 0.16f, 0.25f, 1.00f);
    colors[ImGuiCol_TabHovered]          = ImVec4(0.25f, 0.40f, 0.75f, 1.00f);
    colors[ImGuiCol_TabActive]           = ImVec4(0.20f, 0.32f, 0.60f, 1.00f);

    colors[ImGuiCol_Separator]           = ImVec4(0.30f, 0.35f, 0.50f, 1.00f);
    colors[ImGuiCol_Border]              = ImVec4(0.20f, 0.22f, 0.32f, 1.00f);
    colors[ImGuiCol_Text]                = ImVec4(0.90f, 0.93f, 0.97f, 1.00f);
    colors[ImGuiCol_TextDisabled]        = ImVec4(0.55f, 0.60f, 0.70f, 1.00f);
}
