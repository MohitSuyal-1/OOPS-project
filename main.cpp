#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <SDL_opengl.h>

#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_opengl3.h"
#include "datamanager.h"

#include <vector>
#include <string>
#include <ctime>
#include <random>
#include <algorithm>
#include <cstdio>

#ifdef _WIN32
#include <windows.h>
#endif

// ---------- GLOBAL DATA ----------

DataManager g_dataManager;

enum Page {
    PAGE_ALL_TRAINS    = 1,
    PAGE_SEARCH_NAME   = 2,
    PAGE_SEAT_AVAIL    = 3,
    PAGE_BOOK_TICKET   = 4,
    PAGE_PAYMENT       = 5,   // Fare Summary
    PAGE_VIEW_TICKET   = 6,
    PAGE_CANCEL_TICKET = 7
};

int   g_currentPage = PAGE_ALL_TRAINS;
int   g_prevPage    = -1;
float g_pageAlpha   = 1.0f;

// Buffers
char g_nameSearchBuf[256]    = "";
char g_trainNoAvailBuf[256]  = "";
char g_passengerNameBuf[256] = "";
char g_ageBuf[16]            = "18";
char g_pnrViewBuf[256]       = "";
char g_pnrCancelBuf[256]     = "";

// Lists / selections
std::vector<std::string> g_trainList;
std::vector<std::string> g_classList;
int g_selectedTrainIdx = -1;
int g_selectedClassIdx = -1;

// Results
std::vector<Train>   g_searchResults;
std::vector<Train>   g_availResults;
std::vector<Booking> g_bookingResults;

// Messages
std::string g_message = "";
ImVec4 g_msgColor = ImVec4(1,1,1,1);
bool   g_showMsgPopup = false;

// Booking confirmation (last confirmed)
Booking     g_lastConfirmedBooking;
std::string g_lastBookingId = "";
bool        g_showBookingCard = false;
float       g_bookingCardAnim = 0.0f;

// Pending booking (before fare summary confirmation)
bool        g_hasPendingBooking = false;
Booking     g_pendingBooking;
std::string g_pendingBookingId;

// Forward declarations
void showMessagePopup(const std::string& msg, const ImVec4& color);
void SetupImGuiStyle();
void PopulateTrainList();
void UpdateClassListForTrain(int trainIdx);
std::string GenerateBookingId(const std::string& pnr);
void DrawTopClock();
void DrawGradientBackground(ImGuiViewport* viewport);
void PlayClickSound();

// ---------- HELPERS ----------

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

std::string GenerateBookingId(const std::string& pnr) {
    // Simple derived Booking ID from PNR
    return "BK-" + pnr;
}

void showMessagePopup(const std::string& msg, const ImVec4& color) {
    g_message = msg;
    g_msgColor = color;
    g_showMsgPopup = true;
}

#ifdef _WIN32
void PlayClickSound() {
    Beep(900, 25);
}
#else
void PlayClickSound() {}
#endif

void DrawTopClock() {
    std::time_t now = std::time(nullptr);
    std::tm* lt = std::localtime(&now);
    char buf[32];
    std::strftime(buf, sizeof(buf), "%H:%M:%S", lt);
    ImGui::TextDisabled("%s", buf);
}

void DrawGradientBackground(ImGuiViewport* viewport) {
    ImDrawList* bg = ImGui::GetBackgroundDrawList();
    ImVec2 p0 = viewport->Pos;
    ImVec2 p1 = ImVec2(viewport->Pos.x + viewport->Size.x,
                       viewport->Pos.y + viewport->Size.y);
    ImU32 col_tl = IM_COL32(4, 12, 32, 255);
    ImU32 col_tr = IM_COL32(6, 18, 48, 255);
    ImU32 col_bl = IM_COL32(2, 8, 24, 255);
    ImU32 col_br = IM_COL32(10, 20, 52, 255);
    bg->AddRectFilledMultiColor(p0, p1, col_tl, col_tr, col_br, col_bl);
}

void SetupImGuiStyle() {
    ImGuiStyle& style = ImGui::GetStyle();

    style.WindowRounding     = 10.0f;
    style.ChildRounding      = 10.0f;
    style.FrameRounding      = 8.0f;
    style.ScrollbarRounding  = 8.0f;
    style.GrabRounding       = 8.0f;
    style.TabRounding        = 8.0f;
    style.PopupRounding      = 10.0f;

    style.WindowBorderSize   = 0.0f;
    style.FrameBorderSize    = 0.0f;

    ImVec4* colors = style.Colors;
    colors[ImGuiCol_WindowBg]        = ImVec4(0.04f, 0.05f, 0.09f, 0.95f);
    colors[ImGuiCol_ChildBg]         = ImVec4(0.06f, 0.07f, 0.12f, 0.96f);
    colors[ImGuiCol_Header]          = ImVec4(0.20f, 0.36f, 0.72f, 0.75f);
    colors[ImGuiCol_HeaderHovered]   = ImVec4(0.25f, 0.45f, 0.85f, 0.86f);
    colors[ImGuiCol_HeaderActive]    = ImVec4(0.30f, 0.52f, 0.90f, 1.00f);

    colors[ImGuiCol_Button]          = ImVec4(0.16f, 0.28f, 0.55f, 1.00f);
    colors[ImGuiCol_ButtonHovered]   = ImVec4(0.24f, 0.40f, 0.78f, 1.00f);
    colors[ImGuiCol_ButtonActive]    = ImVec4(0.32f, 0.50f, 0.95f, 1.00f);

    colors[ImGuiCol_FrameBg]         = ImVec4(0.08f, 0.10f, 0.18f, 1.00f);
    colors[ImGuiCol_FrameBgHovered]  = ImVec4(0.18f, 0.30f, 0.55f, 1.00f);
    colors[ImGuiCol_FrameBgActive]   = ImVec4(0.22f, 0.38f, 0.70f, 1.00f);

    colors[ImGuiCol_TitleBg]         = ImVec4(0.05f, 0.06f, 0.10f, 1.00f);
    colors[ImGuiCol_TitleBgActive]   = ImVec4(0.10f, 0.12f, 0.18f, 1.00f);

    colors[ImGuiCol_Tab]             = ImVec4(0.12f, 0.16f, 0.25f, 1.00f);
    colors[ImGuiCol_TabHovered]      = ImVec4(0.25f, 0.40f, 0.75f, 1.00f);
    colors[ImGuiCol_TabActive]       = ImVec4(0.20f, 0.32f, 0.60f, 1.00f);

    colors[ImGuiCol_Separator]       = ImVec4(0.30f, 0.35f, 0.50f, 1.00f);
    colors[ImGuiCol_Border]          = ImVec4(0.20f, 0.22f, 0.32f, 1.00f);

    colors[ImGuiCol_Text]            = ImVec4(0.92f, 0.95f, 0.99f, 1.00f);
    colors[ImGuiCol_TextDisabled]    = ImVec4(0.60f, 0.65f, 0.78f, 1.00f);

    colors[ImGuiCol_PopupBg]         = ImVec4(0.06f, 0.08f, 0.15f, 0.98f);
}

// ---------- MAIN ----------

int main(int argc, char* argv[]) {
    // SDL init
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
        printf("Error: %s\n", SDL_GetError());
        return -1;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

    SDL_DisplayMode current;
    SDL_GetCurrentDisplayMode(0, &current);
    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_FULLSCREEN);
    SDL_Window* window = SDL_CreateWindow(
        "Ticket Reservation System",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        current.w, current.h,
        window_flags
    );

    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, gl_context);
    SDL_GL_SetSwapInterval(1);

    // ImGui init
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.FontGlobalScale = 1.2f; // increase font size ~20%

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
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT)
                done = true;
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        ImGuiViewport* viewport = ImGui::GetMainViewport();

        // Background gradient
        DrawGradientBackground(viewport);

        // Page fade animation
        if (g_currentPage != g_prevPage) {
            g_prevPage = g_currentPage;
            g_pageAlpha = 0.0f;
        }
        g_pageAlpha = std::min(g_pageAlpha + io.DeltaTime * 4.0f, 1.0f);

        ImGui::SetNextWindowPos(viewport->Pos);
        ImGui::SetNextWindowSize(viewport->Size);
        ImGui::Begin("Main",
                     nullptr,
                     ImGuiWindowFlags_NoTitleBar
                     | ImGuiWindowFlags_NoCollapse
                     | ImGuiWindowFlags_NoResize
                     | ImGuiWindowFlags_NoMove
                     | ImGuiWindowFlags_NoBringToFrontOnFocus);

        // HEADER BAR
        {
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(12, 12));
            ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.05f, 0.07f, 0.13f, 0.96f));
            ImGui::BeginChild("HeaderBar", ImVec2(0, 80), true);

            ImGui::TextColored(ImVec4(0.35f, 0.82f, 1.0f, 1.0f),
                               "Ticket Reservation System");
            ImGui::Spacing();
            ImGui::TextDisabled("Smart Reservation Panel");

            // Clock (top-right)
            ImGui::SameLine(ImGui::GetWindowWidth() - 120.0f);
            DrawTopClock();

            ImGui::EndChild();
            ImGui::PopStyleColor();
            ImGui::PopStyleVar();
        }

        ImGui::Spacing();

        float sidebarWidth = 270.0f;
        float footerHeight = ImGui::GetFrameHeightWithSpacing() + 8.0f;

        // SIDEBAR
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.07f, 0.09f, 0.16f, 0.96f));
        ImGui::BeginChild("Sidebar",
                          ImVec2(sidebarWidth, viewport->Size.y - 90.0f - footerHeight),
                          true);

        ImGui::SeparatorText("  MENU  ");
        ImGui::Spacing();

        auto menuButton = [](const char* label, int page) {
            bool active = (g_currentPage == page);

            // Pre-draw glow highlight behind active button
            ImVec2 pos = ImGui::GetCursorScreenPos();
            ImVec2 size(ImGui::GetContentRegionAvail().x, 36.0f);
            ImDrawList* dl = ImGui::GetWindowDrawList();
            if (active) {
                ImVec2 glowMin(pos.x - 4.0f, pos.y - 2.0f);
                ImVec2 glowMax(pos.x + size.x + 4.0f, pos.y + size.y + 2.0f);
                dl->AddRectFilled(glowMin, glowMax,
                                  IM_COL32(40, 100, 220, 140), 18.0f);
            }

            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 18.0f);
            if (active) {
                ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.25f,0.45f,0.90f,1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.30f,0.55f,0.98f,1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.18f,0.36f,0.80f,1.0f));
            }
            bool pressed = ImGui::Button(label, ImVec2(-1, 36.0f));
            if (active) {
                ImGui::PopStyleColor(3);
            }
            ImGui::PopStyleVar();

            if (pressed) {
                PlayClickSound();
                g_currentPage = page;
            }
        };

        menuButton("All Trains",        PAGE_ALL_TRAINS);
        menuButton("Search by Name",    PAGE_SEARCH_NAME);
        menuButton("Seat Availability", PAGE_SEAT_AVAIL);
        menuButton("Book Ticket",       PAGE_BOOK_TICKET);
        menuButton("Fare Summary",      PAGE_PAYMENT);
        menuButton("View Ticket",       PAGE_VIEW_TICKET);
        menuButton("Cancel Ticket",     PAGE_CANCEL_TICKET);

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::TextDisabled("Passenger Safety Tips:");
        ImGui::BulletText("Keep your luggage with you at all times.");
        ImGui::BulletText("Do not share PNR or OTP with strangers.");
        ImGui::BulletText("Verify coach and seat before boarding.");
        ImGui::BulletText("Use official apps / portals only.");

        ImGui::EndChild();
        ImGui::PopStyleColor();

        ImGui::SameLine();

        // CONTENT AREA with fade
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, g_pageAlpha);
        ImGui::BeginChild("Content",
                          ImVec2(0, viewport->Size.y - 90.0f - footerHeight),
                          true);

        switch (g_currentPage) {

            // ---------- ALL TRAINS ----------
            case PAGE_ALL_TRAINS: {
                ImGui::SeparatorText("  ALL TRAINS  ");

                if (ImGui::Button("Reload Trains from CSV", ImVec2(220, 0))) {
                    PlayClickSound();
                    if (g_dataManager.loadTrains()) {
                        PopulateTrainList();
                        showMessagePopup("Train list reloaded from train.csv", ImVec4(0.3f,1.0f,0.3f,1));
                    } else {
                        showMessagePopup("Failed to load train.csv", ImVec4(1,0.3f,0.3f,1));
                    }
                }

                ImGui::Spacing();

                ImGuiTableFlags flags =
                    ImGuiTableFlags_Borders
                    | ImGuiTableFlags_RowBg
                    | ImGuiTableFlags_Resizable
                    | ImGuiTableFlags_SizingStretchProp;

                if (ImGui::BeginTable("TrainsTable", 7, flags)) {
                    ImGui::TableSetupColumn("Train No", ImGuiTableColumnFlags_WidthFixed, 80.0f);
                    ImGui::TableSetupColumn("Train Name");
                    ImGui::TableSetupColumn("From", ImGuiTableColumnFlags_WidthFixed, 110.0f);
                    ImGui::TableSetupColumn("To",   ImGuiTableColumnFlags_WidthFixed, 110.0f);
                    ImGui::TableSetupColumn("Arrival",   ImGuiTableColumnFlags_WidthFixed, 100.0f);
                    ImGui::TableSetupColumn("Departure", ImGuiTableColumnFlags_WidthFixed, 100.0f);
                    ImGui::TableSetupColumn("Classes");

                    ImGui::TableHeadersRow();

                    int rowIndex = 0;
                    for (const auto& train : g_dataManager.trains()) {
                        ImGui::TableNextRow();
                        ImU32 rowBg = (rowIndex % 2 == 0)
                                      ? IM_COL32(16,20,40,180)
                                      : IM_COL32(10,14,30,180);
                        ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, rowBg);

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
                        rowIndex++;
                    }
                    ImGui::EndTable();
                }
                break;
            }

            // ---------- SEARCH BY NAME ----------
            case PAGE_SEARCH_NAME: {
                ImGui::SeparatorText("  SEARCH BY TRAIN NAME  ");
                ImGui::InputText("Train Name", g_nameSearchBuf, IM_ARRAYSIZE(g_nameSearchBuf));
                if (ImGui::Button("Search", ImVec2(140,0))) {
                    PlayClickSound();
                    g_searchResults = g_dataManager.searchTrainsByName(g_nameSearchBuf);
                    if (g_searchResults.empty()) {
                        showMessagePopup("No train found with that name.", ImVec4(1,0.5f,0.2f,1));
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

                    int rowIndex = 0;
                    for (const auto& train : g_searchResults) {
                        ImGui::TableNextRow();
                        ImU32 rowBg = (rowIndex % 2 == 0)
                                      ? IM_COL32(16,20,40,180)
                                      : IM_COL32(10,14,30,180);
                        ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, rowBg);

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

                        rowIndex++;
                    }
                    ImGui::EndTable();
                }
                break;
            }

            // ---------- SEAT AVAILABILITY ----------
            case PAGE_SEAT_AVAIL: {
                ImGui::SeparatorText("  SEAT AVAILABILITY  ");
                ImGui::InputText("Train Number", g_trainNoAvailBuf, IM_ARRAYSIZE(g_trainNoAvailBuf));
                if (ImGui::Button("Check Availability", ImVec2(200,0))) {
                    PlayClickSound();
                    const Train* t = g_dataManager.findTrainByNumber(g_trainNoAvailBuf);
                    if (t) {
                        g_availResults = { *t };
                        showMessagePopup("Availability loaded.", ImVec4(0.3f,1.0f,0.3f,1));
                    } else {
                        g_availResults.clear();
                        showMessagePopup("Train not found.", ImVec4(1,0.3f,0.3f,1));
                    }
                }

                ImGui::Spacing();

                if (!g_availResults.empty()) {
                    const Train& t = g_availResults[0];

                    ImGui::TextColored(ImVec4(0.4f,0.8f,1.0f,1.0f),
                                       "Train: %s - %s", t.trainNo.c_str(), t.trainName.c_str());
                    ImGui::Text("Route: %s -> %s", t.from.c_str(), t.to.c_str());
                    ImGui::Text("Departure: %s", t.dep.c_str());

                    ImGui::Spacing();

                    if (ImGui::BeginTable("AvailTable", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
                        ImGui::TableSetupColumn("Class");
                        ImGui::TableSetupColumn("Total");
                        ImGui::TableSetupColumn("Booked");
                        ImGui::TableSetupColumn("Available");
                        ImGui::TableSetupColumn("Fare (Rs)");
                        ImGui::TableHeadersRow();

                        int rowIndex = 0;
                        for (const auto& cls : t.classes) {
                            int total  = g_dataManager.seatCapacity().at(cls);
                            int booked = g_dataManager.bookedCount(t.trainNo, cls);
                            int avail  = total - booked;
                            int fare   = g_dataManager.fares().at(cls);

                            ImGui::TableNextRow();
                            ImU32 rowBg = (rowIndex % 2 == 0)
                                          ? IM_COL32(16,20,40,180)
                                          : IM_COL32(10,14,30,180);
                            ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, rowBg);

                            ImGui::TableNextColumn(); ImGui::Text("%s", cls.c_str());
                            ImGui::TableNextColumn(); ImGui::Text("%d", total);
                            ImGui::TableNextColumn(); ImGui::Text("%d", booked);
                            ImGui::TableNextColumn(); ImGui::Text("%d", avail);
                            ImGui::TableNextColumn(); ImGui::Text("%d", fare);

                            rowIndex++;
                        }
                        ImGui::EndTable();
                    }
                }
                break;
            }

            // ---------- BOOK TICKET ----------
            case PAGE_BOOK_TICKET: {
                ImGui::SeparatorText("  BOOK TICKET  ");

                ImGui::Columns(2, nullptr, false);

                // Left: passenger + train selection
                ImGui::Text("Passenger Details");
                ImGui::Separator();
                ImGui::InputText("Name", g_passengerNameBuf, IM_ARRAYSIZE(g_passengerNameBuf));
                ImGui::InputText("Age",  g_ageBuf,           IM_ARRAYSIZE(g_ageBuf));

                ImGui::Spacing();
                ImGui::Text("Select Train:");
                if (ImGui::BeginListBox("##trainlist", ImVec2(-FLT_MIN, 180))) {
                    for (int i = 0; i < (int)g_trainList.size(); i++) {
                        const bool is_selected = (g_selectedTrainIdx == i);
                        if (ImGui::Selectable(g_trainList[i].c_str(), is_selected)) {
                            PlayClickSound();
                            g_selectedTrainIdx = i;
                            UpdateClassListForTrain(i);
                        }
                        if (is_selected) {
                            ImDrawList* dl = ImGui::GetWindowDrawList();
                            ImVec2 min = ImGui::GetItemRectMin();
                            ImVec2 max = ImGui::GetItemRectMax();
                            dl->AddRect(min, max, IM_COL32(60,130,255,255), 6.0f, 0, 2.0f);
                            ImGui::SetItemDefaultFocus();
                        }
                    }
                    ImGui::EndListBox();
                }

                if (g_selectedTrainIdx >= 0) {
                    const Train& t = g_dataManager.trains()[g_selectedTrainIdx];
                    ImGui::Spacing();
                    ImGui::TextColored(ImVec4(0.9f,0.85f,0.3f,1.0f),
                                       "%s  (%s -> %s)", t.trainNo.c_str(), t.from.c_str(), t.to.c_str());
                    ImGui::Text("Departure: %s", t.dep.c_str());
                }

                ImGui::NextColumn();

                // Right: class & fare preview
                ImGui::Text("Class & Fare");
                ImGui::Separator();

                ImGui::Text("Select Class:");
                for (int i = 0; i < (int)g_classList.size(); i++) {
                    bool is_selected = (g_selectedClassIdx == i);
                    if (ImGui::RadioButton(g_classList[i].c_str(), is_selected)) {
                        PlayClickSound();
                        g_selectedClassIdx = i;
                    }
                    if ((i + 1) % 4 != 0) ImGui::SameLine();
                }

                ImGui::Spacing();

                ImGui::BeginChild("FarePreview", ImVec2(0, 120), true);
                ImGui::TextColored(ImVec4(0.4f,0.85f,1.0f,1.0f), "Fare Preview");
                ImGui::Separator();
                if (g_selectedTrainIdx >= 0 && g_selectedClassIdx >= 0) {
                    const Train& t = g_dataManager.trains()[g_selectedTrainIdx];
                    std::string cls = g_classList[g_selectedClassIdx];
                    int fare = g_dataManager.fares().at(cls);
                    int avail = g_dataManager.seatCapacity().at(cls)
                                - g_dataManager.bookedCount(t.trainNo, cls);

                    ImGui::Text("Train: %s - %s", t.trainNo.c_str(), t.trainName.c_str());
                    ImGui::Text("Class: %s", cls.c_str());
                    ImGui::Text("Fare:  Rs. %d", fare);
                    ImGui::Text("Seats Left: %d", avail);
                } else {
                    ImGui::TextDisabled("Select a train and class to see fare.");
                }
                ImGui::EndChild();

                ImGui::Columns(1);
                ImGui::Spacing();

                if (ImGui::Button("Proceed to Fare Summary", ImVec2(260, 0))) {
                    PlayClickSound();
                    if (strlen(g_passengerNameBuf) == 0) {
                        showMessagePopup("Please enter passenger name.", ImVec4(1,0.3f,0.3f,1));
                    } else {
                        int age;
                        try {
                            age = std::stoi(g_ageBuf);
                            if (age < 1 || age > 120) throw std::exception();
                        } catch (...) {
                            showMessagePopup("Please enter a valid age (1-120).", ImVec4(1,0.3f,0.3f,1));
                            break;
                        }
                        if (g_selectedTrainIdx < 0 || g_selectedClassIdx < 0) {
                            showMessagePopup("Select train and class.", ImVec4(1,0.3f,0.3f,1));
                            break;
                        }

                        const Train& t = g_dataManager.trains()[g_selectedTrainIdx];
                        std::string cls = g_classList[g_selectedClassIdx];
                        int avail = g_dataManager.seatCapacity().at(cls)
                                    - g_dataManager.bookedCount(t.trainNo, cls);
                        if (avail <= 0) {
                            showMessagePopup("No seats available in this class.", ImVec4(1,0.3f,0.3f,1));
                            break;
                        }

                        // Prepare pending booking (NOT saved yet)
                        g_pendingBooking.pnr       = g_dataManager.generatePNR();
                        g_pendingBooking.name      = g_passengerNameBuf;
                        g_pendingBooking.age       = age;
                        g_pendingBooking.trainNo   = t.trainNo;
                        g_pendingBooking.trainName = t.trainName;
                        g_pendingBooking.classType = cls;
                        g_pendingBooking.seatNo    = g_dataManager.nextSeatNo(t.trainNo, cls);
                        g_pendingBooking.fare      = g_dataManager.fares().at(cls);
                        g_pendingBooking.departure = t.dep;

                        g_pendingBookingId   = GenerateBookingId(g_pendingBooking.pnr);
                        g_hasPendingBooking  = true;

                        g_currentPage = PAGE_PAYMENT;  // go to Fare Summary
                    }
                }

                break;
            }

            // ---------- PAYMENT / FARE SUMMARY ----------
            case PAGE_PAYMENT: {
                ImGui::SeparatorText("  FARE SUMMARY  ");

                if (!g_hasPendingBooking) {
                    ImGui::TextDisabled("No pending booking. Please book a ticket first.");
                    break;
                }

                // Centered IRCTC-style blue card
                float availWidth = ImGui::GetContentRegionAvail().x;
                float cardWidth  = std::min(availWidth - 60.0f, 550.0f);
                float startX     = (availWidth - cardWidth) * 0.5f;

                ImGui::SetCursorPosX(ImGui::GetCursorPosX() + startX);
                ImGui::BeginChild("FareCard", ImVec2(cardWidth, 260.0f), true);

                // Header strip
                ImGui::TextColored(ImVec4(0.4f, 0.9f, 1.0f, 1.0f), "Fare Summary");
                ImGui::Separator();

                ImGui::Columns(2, nullptr, false);
                ImGui::Text("Booking ID: %s", g_pendingBookingId.c_str());
                ImGui::Text("PNR: %s", g_pendingBooking.pnr.c_str());
                ImGui::Text("Passenger: %s", g_pendingBooking.name.c_str());
                ImGui::Text("Age: %d", g_pendingBooking.age);
                ImGui::NextColumn();
                ImGui::Text("Train: %s - %s",
                            g_pendingBooking.trainNo.c_str(),
                            g_pendingBooking.trainName.c_str());
                ImGui::Text("Class: %s", g_pendingBooking.classType.c_str());
                ImGui::Text("Seat: %d", g_pendingBooking.seatNo);
                ImGui::Text("Departure: %s", g_pendingBooking.departure.c_str());
                ImGui::Columns(1);

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();
                ImGui::TextColored(ImVec4(0.2f, 0.9f, 0.4f, 1.0f),
                                   "Ticket Fare: \xE2\x82\xB9 %d", g_pendingBooking.fare);

                ImGui::EndChild();

                ImGui::Spacing();
                ImGui::SetCursorPosX(ImGui::GetCursorPosX() + startX);
                if (ImGui::Button("Confirm Booking", ImVec2(cardWidth, 0))) {
                    PlayClickSound();
                    // FINAL BOOKING SAVE HERE
                    if (!g_dataManager.addBooking(g_pendingBooking)) {
                        showMessagePopup("Failed to save booking.", ImVec4(1,0.3f,0.3f,1));
                    } else {
                        g_lastConfirmedBooking = g_pendingBooking;
                        g_lastBookingId        = g_pendingBookingId;
                        g_hasPendingBooking    = false;

                        // Pre-fill View Ticket PNR
                        std::snprintf(g_pnrViewBuf, sizeof(g_pnrViewBuf),
                                      "%s", g_lastConfirmedBooking.pnr.c_str());

                        g_bookingResults.clear();
                        g_bookingResults.push_back(g_lastConfirmedBooking);

                        // Show confirmation popup card
                        g_showBookingCard = true;
                        g_bookingCardAnim = 0.0f;

                        g_currentPage = PAGE_VIEW_TICKET;
                    }
                }

                break;
            }

            // ---------- VIEW TICKET ----------
            case PAGE_VIEW_TICKET: {
                ImGui::SeparatorText("  VIEW TICKET  ");
                ImGui::InputText("Enter PNR", g_pnrViewBuf, IM_ARRAYSIZE(g_pnrViewBuf));
                if (ImGui::Button("Search Ticket", ImVec2(180,0))) {
                    PlayClickSound();
                    g_bookingResults = g_dataManager.findBookingsByPNR(g_pnrViewBuf);
                    if (g_bookingResults.empty()) {
                        showMessagePopup("No ticket found for this PNR.", ImVec4(1,0.3f,0.3f,1));
                    }
                }

                ImGui::Spacing();

                if (!g_bookingResults.empty()) {
                    const Booking& b = g_bookingResults[0];

                    float availWidth = ImGui::GetContentRegionAvail().x;
                    float cardWidth  = std::min(availWidth - 60.0f, 550.0f);
                    float startX     = (availWidth - cardWidth) * 0.5f;

                    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + startX);
                    ImGui::BeginChild("TicketCard", ImVec2(cardWidth, 230), true);

                    ImGui::TextColored(ImVec4(0.3f,1.0f,1.0f,1.0f), "Ticket Details");
                    ImGui::Separator();

                    ImGui::Columns(2, nullptr, false);
                    ImGui::Text("PNR: %s", b.pnr.c_str());
                    std::string bookingId = GenerateBookingId(b.pnr);
                    ImGui::Text("Booking ID: %s", bookingId.c_str());
                    ImGui::Text("Passenger: %s (Age: %d)", b.name.c_str(), b.age);
                    ImGui::Text("Train: %s - %s", b.trainNo.c_str(), b.trainName.c_str());
                    ImGui::NextColumn();
                    ImGui::Text("Class: %s", b.classType.c_str());
                    ImGui::Text("Seat: %d", b.seatNo);
                    ImGui::Text("Fare: Rs. %d", b.fare);
                    ImGui::Text("Departure: %s", b.departure.c_str());
                    ImGui::Columns(1);

                    ImGui::EndChild();
                }

                break;
            }

            // ---------- CANCEL TICKET ----------
            case PAGE_CANCEL_TICKET: {
                ImGui::SeparatorText("  CANCEL TICKET  ");
                ImGui::InputText("Enter PNR", g_pnrCancelBuf, IM_ARRAYSIZE(g_pnrCancelBuf));
                if (ImGui::Button("Find Ticket", ImVec2(160,0))) {
                    PlayClickSound();
                    g_bookingResults = g_dataManager.findBookingsByPNR(g_pnrCancelBuf);
                    if (g_bookingResults.empty()) {
                        showMessagePopup("No ticket found for this PNR.", ImVec4(1,0.3f,0.3f,1));
                    }
                }

                ImGui::Spacing();

                if (!g_bookingResults.empty()) {
                    const Booking& b = g_bookingResults[0];

                    ImGui::BeginChild("CancelCard", ImVec2(0, 230), true);
                    ImGui::TextColored(ImVec4(0.3f,1.0f,1.0f,1.0f), "Ticket Found");
                    ImGui::Separator();

                    ImGui::Columns(2, nullptr, false);
                    ImGui::Text("PNR: %s", b.pnr.c_str());
                    ImGui::Text("Passenger: %s (Age: %d)", b.name.c_str(), b.age);
                    ImGui::Text("Train: %s - %s", b.trainNo.c_str(), b.trainName.c_str());
                    ImGui::NextColumn();
                    ImGui::Text("Class: %s", b.classType.c_str());
                    ImGui::Text("Seat: %d", b.seatNo);
                    ImGui::Text("Fare: Rs. %d", b.fare);
                    ImGui::Text("Departure: %s", b.departure.c_str());
                    ImGui::Columns(1);

                    ImGui::Spacing();
                    if (ImGui::Button("Confirm Cancellation", ImVec2(220,0))) {
                        PlayClickSound();
                        Booking removed;
                        if (g_dataManager.cancelBooking(b.pnr, &removed)) {
                            showMessagePopup("Ticket cancelled successfully.", ImVec4(0.3f,1.0f,0.3f,1));
                            memset(g_pnrCancelBuf, 0, sizeof(g_pnrCancelBuf));
                            g_bookingResults.clear();
                        } else {
                            showMessagePopup("Failed to cancel ticket.", ImVec4(1,0.3f,0.3f,1));
                        }
                    }

                    ImGui::EndChild();
                }

                break;
            }
        }

        ImGui::EndChild(); // Content
        ImGui::PopStyleVar(); // page alpha

        // FOOTER
        ImGui::SetCursorPosY(viewport->Size.y - footerHeight);
        ImGui::Separator();
        ImGui::TextDisabled("Smart Reservation Panel  |  Please keep your luggage safe during travel.");

        ImGui::End(); // Main window

        // ---------- BOOKING CONFIRMED POPUP CARD ----------
        if (g_showBookingCard) {
            ImGui::OpenPopup("Booking Confirmed!");
            g_showBookingCard = false;
        }
        if (ImGui::BeginPopupModal("Booking Confirmed!", nullptr,
                                   ImGuiWindowFlags_AlwaysAutoResize)) {
            g_bookingCardAnim = std::min(g_bookingCardAnim + io.DeltaTime * 4.0f, 1.0f);
            float scale = 1.0f + 0.05f * (1.0f - g_bookingCardAnim);
            ImGui::SetWindowFontScale(scale);

            ImGui::TextColored(ImVec4(0.4f,1.0f,1.0f,1.0f), "Booking Confirmed!");
            ImGui::Separator();

            std::string bid = g_lastBookingId.empty()
                              ? GenerateBookingId(g_lastConfirmedBooking.pnr)
                              : g_lastBookingId;

            ImGui::Columns(2, nullptr, false);
            ImGui::Text("Booking ID: %s", bid.c_str());
            ImGui::Text("PNR: %s", g_lastConfirmedBooking.pnr.c_str());
            ImGui::Text("Passenger: %s", g_lastConfirmedBooking.name.c_str());
            ImGui::Text("Age: %d", g_lastConfirmedBooking.age);
            ImGui::NextColumn();
            ImGui::Text("Train: %s - %s",
                        g_lastConfirmedBooking.trainNo.c_str(),
                        g_lastConfirmedBooking.trainName.c_str());
            ImGui::Text("Class: %s", g_lastConfirmedBooking.classType.c_str());
            ImGui::Text("Seat: %d", g_lastConfirmedBooking.seatNo);
            ImGui::Text("Fare: Rs. %d", g_lastConfirmedBooking.fare);
            ImGui::Text("Departure: %s", g_lastConfirmedBooking.departure.c_str());
            ImGui::Columns(1);

            ImGui::Spacing();
            ImGui::SetCursorPosX((ImGui::GetWindowSize().x - 130.0f) * 0.5f);
            if (ImGui::Button("OK", ImVec2(130, 0))) {
                PlayClickSound();
                ImGui::SetWindowFontScale(1.0f);
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
            ImGui::SetWindowFontScale(1.0f);
        }

        // ---------- GENERIC MESSAGE POPUP ----------
        if (g_showMsgPopup) {
            ImGui::OpenPopup("Message");
            g_showMsgPopup = false;
        }
        if (ImGui::BeginPopupModal("Message", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::TextColored(g_msgColor, "%s", g_message.c_str());
            ImGui::Spacing();
            if (ImGui::Button("OK", ImVec2(120, 0))) {
                PlayClickSound();
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        // Rendering
        ImGui::Render();
        glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
        glClearColor(0,0,0,1);
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
