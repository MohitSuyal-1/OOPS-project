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

int g_currentPage = 1; // pages: 1–6

// Buffers
char g_nameSearchBuf[256] = "";
char g_trainNoAvailBuf[256] = "";
char g_passengerNameBuf[256] = "";
char g_ageBuf[10] = "18";
char g_pnrViewBuf[256] = "";
char g_pnrCancelBuf[256] = "";

// Train selection
int g_selectedTrainIdx = -1;
int g_selectedClassIdx = -1;
std::vector<std::string> g_trainList;
std::vector<std::string> g_classList;

// Results
std::vector<Train> g_searchResults;
std::vector<Train> g_availResults;
std::vector<Booking> g_bookingResults;

// Messages
std::string g_message = "";
ImVec4 g_msgColor = ImVec4(1, 1, 1, 1);
bool g_showMsgPopup = false;

// Functions
void showMessagePopup(const std::string& msg, const ImVec4& color);
void SetupImGuiStyle();

void PopulateTrainList() {
    g_trainList.clear();
    for (const auto& t : g_dataManager.trains()) {
        g_trainList.push_back(t.trainNo + " - " + t.trainName);
    }
}

void UpdateClassListForTrain(int trainIdx) {
    if (trainIdx < 0) return;
    g_classList.clear();
    for (const auto& c : g_dataManager.trains()[trainIdx].classes) {
        g_classList.push_back(c);
    }
    g_selectedClassIdx = -1;
}

int main(int, char**) {
    // SDL Init
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
        printf("SDL Error: %s\n", SDL_GetError());
        return -1;
    }

    // GL settings
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

    SDL_DisplayMode dm;
    SDL_GetCurrentDisplayMode(0, &dm);

    SDL_Window* window = SDL_CreateWindow(
        "Ticket Reservation System",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        dm.w, dm.h,
        SDL_WINDOW_OPENGL | SDL_WINDOW_FULLSCREEN);

    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, gl_context);
    SDL_GL_SetSwapInterval(1);

    // ImGui Init
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.FontGlobalScale = 1.2f;

    SetupImGuiStyle();
    ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init("#version 130");

    // Load data
    g_dataManager.setTrainFilePath("train.csv");
    g_dataManager.setBookingFilePath("booking.csv");
    g_dataManager.loadTrains();
    g_dataManager.loadBookings();
    PopulateTrainList();

    bool done = false;
    while (!done) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            ImGui_ImplSDL2_ProcessEvent(&e);
            if (e.type == SDL_QUIT) done = true;
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        // Main UI window full screen
        ImGuiViewport* vp = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(vp->Pos);
        ImGui::SetNextWindowSize(vp->Size);

        ImGui::Begin("Main",
                     nullptr,
                     ImGuiWindowFlags_NoTitleBar |
                     ImGuiWindowFlags_NoResize |
                     ImGuiWindowFlags_NoMove);

        // HEADER
        {
            ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.07f, 0.09f, 0.18f, 1));
            ImGui::BeginChild("Header", ImVec2(0, 70), true);

            ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1), "Ticket Reservation System");
            ImGui::TextDisabled("Smart Reservation Panel");

            ImGui::EndChild();
            ImGui::PopStyleColor();
        }

        ImGui::Spacing();

        // LAYOUT
        float sidebarWidth = 260.0f;
        float footerHeight = 40;

        // SIDEBAR
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.08f, 0.10f, 0.18f, 1));
        ImGui::BeginChild("Sidebar",
                          ImVec2(sidebarWidth, vp->Size.y - 110),
                          true);

        ImGui::SeparatorText(" MENU ");

        auto menuButton = [&](const char* txt, int page) {
            bool active = (g_currentPage == page);
            if (active) {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.24f, 0.48f, 0.82f, 1));
            }

            if (ImGui::Button(txt, ImVec2(-1, 0))) {
                g_currentPage = page;
            }

            if (active) ImGui::PopStyleColor();
        };

        menuButton("All Trains",        1);
        menuButton("Search by Name",    2);
        menuButton("Seat Availability", 3);
        menuButton("Book Ticket",       4);
        menuButton("View Ticket",       5);
        menuButton("Cancel Ticket",     6);

        ImGui::Separator();
        ImGui::TextDisabled("Passenger Safety Tips:");
        ImGui::BulletText("Keep luggage with you always.");
        ImGui::BulletText("Do not share PNR with strangers.");
        ImGui::BulletText("Verify coach & seat before boarding.");

        ImGui::EndChild();
        ImGui::PopStyleColor();

        ImGui::SameLine();

        // CONTENT AREA
        ImGui::BeginChild("Content", ImVec2(0, vp->Size.y - 110), true);

        // ================= PAGES =====================
        switch (g_currentPage) {

        // PAGE 1 – ALL TRAINS
        case 1: {
            ImGui::SeparatorText(" ALL TRAINS ");

            if (ImGui::Button("Reload CSV")) {
                if (g_dataManager.loadTrains()) {
                    PopulateTrainList();
                    showMessagePopup("Reloaded trains from CSV.", ImVec4(0.2f, 1, 0.2f, 1));
                }
            }

            if (ImGui::BeginTable("TrainTable", 7, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
                ImGui::TableSetupColumn("Train No");
                ImGui::TableSetupColumn("Train Name");
                ImGui::TableSetupColumn("From");
                ImGui::TableSetupColumn("To");
                ImGui::TableSetupColumn("Arrival");
                ImGui::TableSetupColumn("Departure");
                ImGui::TableSetupColumn("Classes");
                ImGui::TableHeadersRow();

                for (auto& t : g_dataManager.trains()) {
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn(); ImGui::Text("%s", t.trainNo.c_str());
                    ImGui::TableNextColumn(); ImGui::Text("%s", t.trainName.c_str());
                    ImGui::TableNextColumn(); ImGui::Text("%s", t.from.c_str());
                    ImGui::TableNextColumn(); ImGui::Text("%s", t.to.c_str());
                    ImGui::TableNextColumn(); ImGui::Text("%s", t.arr.c_str());
                    ImGui::TableNextColumn(); ImGui::Text("%s", t.dep.c_str());

                    ImGui::TableNextColumn();
                    std::string cls;
                    for (auto& c : t.classes) { cls += c + " "; }
                    ImGui::Text("%s", cls.c_str());
                }

                ImGui::EndTable();
            }
        } break;

        // PAGE 2 – SEARCH BY NAME
        case 2: {
            ImGui::SeparatorText(" SEARCH BY TRAIN NAME ");

            ImGui::InputText("Train Name", g_nameSearchBuf, 256);

            if (ImGui::Button("Search")) {
                g_searchResults = g_dataManager.searchTrainsByName(g_nameSearchBuf);
                if (g_searchResults.empty()) {
                    showMessagePopup("No trains found.", ImVec4(1, 0.4f, 0.3f, 1));
                }
            }

            if (ImGui::BeginTable("NameSearch", 7, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
                ImGui::TableSetupColumn("Train No");
                ImGui::TableSetupColumn("Name");
                ImGui::TableSetupColumn("From");
                ImGui::TableSetupColumn("To");
                ImGui::TableSetupColumn("Arrival");
                ImGui::TableSetupColumn("Departure");
                ImGui::TableSetupColumn("Classes");
                ImGui::TableHeadersRow();

                for (auto& t : g_searchResults) {
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn(); ImGui::Text("%s", t.trainNo.c_str());
                    ImGui::TableNextColumn(); ImGui::Text("%s", t.trainName.c_str());
                    ImGui::TableNextColumn(); ImGui::Text("%s", t.from.c_str());
                    ImGui::TableNextColumn(); ImGui::Text("%s", t.to.c_str());
                    ImGui::TableNextColumn(); ImGui::Text("%s", t.arr.c_str());
                    ImGui::TableNextColumn(); ImGui::Text("%s", t.dep.c_str());

                    std::string cls;
                    for (auto& c : t.classes) cls += c + " ";
                    ImGui::TableNextColumn(); ImGui::Text("%s", cls.c_str());
                }

                ImGui::EndTable();
            }
        } break;

        // PAGE 3 – SEAT AVAILABILITY
        case 3: {
            ImGui::SeparatorText(" SEAT AVAILABILITY ");

            ImGui::InputText("Train Number", g_trainNoAvailBuf, 256);
            if (ImGui::Button("Check")) {
                const Train* t = g_dataManager.findTrainByNumber(g_trainNoAvailBuf);
                if (t) {
                    g_availResults = { *t };
                } else {
                    g_availResults.clear();
                    showMessagePopup("Train not found.", ImVec4(1, 0.4f, 0.3f, 1));
                }
            }

            if (!g_availResults.empty()) {
                auto& t = g_availResults[0];

                ImGui::Text("Train: %s - %s", t.trainNo.c_str(), t.trainName.c_str());
                ImGui::Text("Route: %s -> %s", t.from.c_str(), t.to.c_str());
                ImGui::Text("Departure: %s", t.dep.c_str());

                if (ImGui::BeginTable("Avail", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
                    ImGui::TableSetupColumn("Class");
                    ImGui::TableSetupColumn("Total");
                    ImGui::TableSetupColumn("Booked");
                    ImGui::TableSetupColumn("Available");
                    ImGui::TableSetupColumn("Fare");
                    ImGui::TableHeadersRow();

                    for (auto& cls : t.classes) {
                        int total = g_dataManager.seatCapacity().at(cls);
                        int booked = g_dataManager.bookedCount(t.trainNo, cls);
                        int avail = total - booked;
                        int fare = g_dataManager.fares().at(cls);

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
        } break;

        // PAGE 4 – BOOK TICKET
        case 4: {
            ImGui::SeparatorText(" BOOK TICKET ");

            ImGui::InputText("Passenger Name", g_passengerNameBuf, 256);
            ImGui::InputText("Age", g_ageBuf, 10);

            ImGui::SeparatorText("Select Train");

            if (ImGui::BeginListBox("##trains", ImVec2(-FLT_MIN, 150))) {
                for (int i = 0; i < g_trainList.size(); i++) {
                    bool sel = (g_selectedTrainIdx == i);
                    if (ImGui::Selectable(g_trainList[i].c_str(), sel)) {
                        g_selectedTrainIdx = i;
                        UpdateClassListForTrain(i);
                    }
                }
                ImGui::EndListBox();
            }

            if (g_selectedTrainIdx >= 0) {
                auto& t = g_dataManager.trains()[g_selectedTrainIdx];
                ImGui::Text("Train: %s - %s", t.trainNo.c_str(), t.trainName.c_str());
            }

            ImGui::SeparatorText("Select Class");

            for (int i = 0; i < g_classList.size(); i++) {
                if (ImGui::RadioButton(g_classList[i].c_str(), g_selectedClassIdx == i)) {
                    g_selectedClassIdx = i;
                }
                ImGui::SameLine();
            }
            ImGui::NewLine();

            if (g_selectedClassIdx >= 0) {
                auto cls = g_classList[g_selectedClassIdx];
                int fare = g_dataManager.fares().at(cls);
                ImGui::Text("Fare: %d", fare);
            }

            if (ImGui::Button("Confirm Booking", ImVec2(200, 0))) {
                if (strlen(g_passengerNameBuf) == 0) {
                    showMessagePopup("Enter passenger name.", ImVec4(1, 0.3f, 0.3f, 1));
                    break;
                }

                int age = atoi(g_ageBuf);
                if (age < 1 || age > 120) {
                    showMessagePopup("Invalid age.", ImVec4(1, 0.3f, 0.3f, 1));
                    break;
                }

                if (g_selectedTrainIdx < 0 || g_selectedClassIdx < 0) {
                    showMessagePopup("Select train & class.", ImVec4(1, 0.3f, 0.3f, 1));
                    break;
                }

                auto& t = g_dataManager.trains()[g_selectedTrainIdx];
                auto cls = g_classList[g_selectedClassIdx];
                int avail =
                    g_dataManager.seatCapacity().at(cls) -
                    g_dataManager.bookedCount(t.trainNo, cls);

                if (avail <= 0) {
                    showMessagePopup("No seats available.", ImVec4(1, 0.3f, 0.3f, 1));
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
                    showMessagePopup("Booking confirmed!", ImVec4(0.2f, 1, 0.4f, 1));

                    memset(g_passengerNameBuf, 0, sizeof(g_passengerNameBuf));
                    strcpy(g_ageBuf, "18");
                    g_selectedTrainIdx = -1;
                    g_selectedClassIdx = -1;
                    g_classList.clear();
                }
            }
        } break;

        // PAGE 5 – VIEW TICKET
        case 5: {
            ImGui::SeparatorText(" VIEW TICKET ");
            ImGui::InputText("Enter PNR", g_pnrViewBuf, 256);

            if (ImGui::Button("Search")) {
                g_bookingResults = g_dataManager.findBookingsByPNR(g_pnrViewBuf);
                if (g_bookingResults.empty()) {
                    showMessagePopup("Ticket not found.", ImVec4(1, 0.3f, 0.3f, 1));
                }
            }

            if (!g_bookingResults.empty()) {
                auto& b = g_bookingResults[0];

                ImGui::BeginChild("TicketCard", ImVec2(0, 0), true);
                ImGui::TextColored(ImVec4(0.4f, 1, 1, 1), "Ticket Details");
                ImGui::Separator();
                ImGui::Text("PNR: %s", b.pnr.c_str());
                ImGui::Text("Name: %s (Age: %d)", b.name.c_str(), b.age);
                ImGui::Text("Train: %s - %s", b.trainNo.c_str(), b.trainName.c_str());
                ImGui::Text("Class: %s", b.classType.c_str());
                ImGui::Text("Seat: %d", b.seatNo);
                ImGui::Text("Fare: Rs %d", b.fare);
                ImGui::Text("Departure: %s", b.departure.c_str());
                ImGui::EndChild();
            }
        } break;

        // PAGE 6 – CANCEL TICKET
        case 6: {
            ImGui::SeparatorText(" CANCEL TICKET ");

            ImGui::InputText("Enter PNR", g_pnrCancelBuf, 256);

            if (ImGui::Button("Find")) {
                g_bookingResults = g_dataManager.findBookingsByPNR(g_pnrCancelBuf);
                if (g_bookingResults.empty()) {
                    showMessagePopup("Ticket not found.", ImVec4(1, 0.3f, 0.3f, 1));
                }
            }

            if (!g_bookingResults.empty()) {
                auto& b = g_bookingResults[0];

                ImGui::BeginChild("CancelCard", ImVec2(0, 0), true);
                ImGui::Text("PNR: %s", b.pnr.c_str());
                ImGui::Text("Passenger: %s (%d)", b.name.c_str(), b.age);
                ImGui::Text("Train: %s - %s", b.trainNo.c_str(), b.trainName.c_str());
                ImGui::Text("Class: %s", b.classType.c_str());
                ImGui::Spacing();

                if (ImGui::Button("Confirm Cancellation", ImVec2(240, 0))) {
                    if (g_dataManager.cancelBooking(b.pnr, nullptr)) {
                        showMessagePopup("Ticket cancelled.", ImVec4(0.2f, 1, 0.3f, 1));
                        memset(g_pnrCancelBuf, 0, sizeof(g_pnrCancelBuf));
                        g_bookingResults.clear();
                    }
                }

                ImGui::EndChild();
            }
        } break;

        }

        ImGui::EndChild();

        // FOOTER
        ImGui::SetCursorPosY(vp->Size.y - 35);
        ImGui::Separator();
        ImGui::TextDisabled("Smart Reservation Panel  |  Keep luggage safe while travelling.");

        ImGui::End();

        // Popup
        if (g_showMsgPopup) {
            ImGui::OpenPopup("Message");
            g_showMsgPopup = false;
        }

        if (ImGui::BeginPopupModal("Message", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::TextColored(g_msgColor, "%s", g_message.c_str());
            if (ImGui::Button("OK", ImVec2(120, 0))) {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        // Render
        ImGui::Render();
        glViewport(0, 0, io.DisplaySize.x, io.DisplaySize.y);
        glClearColor(0.05f, 0.06f, 0.10f, 1);
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
    ImGuiStyle& s = ImGui::GetStyle();
    s.WindowRounding = 8;
    s.ChildRounding = 8;
    s.FrameRounding = 6;

    ImVec4* c = s.Colors;
    c[ImGuiCol_WindowBg] = ImVec4(0.06f, 0.07f, 0.12f, 1);
    c[ImGuiCol_ChildBg]  = ImVec4(0.07f, 0.08f, 0.13f, 1);
    c[ImGuiCol_Button]   = ImVec4(0.18f, 0.30f, 0.55f, 1);
    c[ImGuiCol_ButtonHovered] = ImVec4(0.24f, 0.40f, 0.75f, 1);
    c[ImGuiCol_Text] = ImVec4(0.9f, 0.93f, 0.97f, 1);
    c[ImGuiCol_TextDisabled] = ImVec4(0.6f, 0.65f, 0.75f, 1);
}
