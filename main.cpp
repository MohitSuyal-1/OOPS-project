// ---------------- MAIN.CPP (WORKING SIMPLE VERSION) ----------------
#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <SDL_opengl.h>

#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_opengl3.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include <set>
#include <string>
#include <cstdlib>
#include <ctime>

using namespace std;

// ---------------- SIMPLE TRAIN & BOOKING STRUCT ----------------
struct Train {
    string trainNo, trainName, from, to, arr, dep, stop;
    set<string> classes;
};

struct Booking {
    string pnr, name, trainNo, trainName, classType, departure;
    int age, seatNo, fare;
};

// ---------------- SIMPLE DATABASE (NO ADVANCED C++) ----------------
class Database {
public:
    vector<Train> trains;
    vector<Booking> bookings;
    map<string,int> seatCapacity;
    map<string,int> fares;

    Database() {
        seatCapacity["1A"]=20; seatCapacity["2A"]=40; seatCapacity["3A"]=60;
        seatCapacity["3E"]=70; seatCapacity["SL"]=120; seatCapacity["CC"]=80; seatCapacity["2S"]=100;

        fares["1A"]=2000; fares["2A"]=1500; fares["3A"]=1100;
        fares["3E"]=900; fares["SL"]=400; fares["CC"]=700; fares["2S"]=300;

        srand((unsigned)time(0));
    }

    string trim(string s) {
        size_t a = s.find_first_not_of(" \t\r\n");
        size_t b = s.find_last_not_of(" \t\r\n");
        if (a == string::npos) return "";
        return s.substr(a, b - a + 1);
    }

    vector<string> split(string s, char d) {
        vector<string> out;
        string temp="";
        for (int i=0;i<s.size();i++) {
            if (s[i]==d) { out.push_back(trim(temp)); temp=""; }
            else temp+=s[i];
        }
        out.push_back(trim(temp));
        return out;
    }

    bool loadTrains() {
        trains.clear();
        ifstream f("trains.csv");
        if (!f.is_open()) return false;

        string line;
        getline(f,line); // skip header

        while(getline(f,line)) {
            if (line=="") continue;

            vector<string> p = split(line, ',');
            if (p.size()<8) continue;

            Train t;
            t.trainNo=p[0]; t.trainName=p[1]; t.from=p[2]; t.to=p[3];
            t.arr=p[4]; t.dep=p[5]; t.stop=p[6];

            stringstream ss(p[7]);
            string c;
            while (ss>>c) t.classes.insert(c);

            trains.push_back(t);
        }
        return true;
    }

    bool loadBookings() {
        bookings.clear();
        ifstream f("bookings.csv");
        if (!f.is_open()) return true;

        string line;
        getline(f,line); // skip header

        while(getline(f,line)) {
            if (line=="") continue;
            vector<string> p = split(line, ',');
            if (p.size()<9) continue;

            Booking b;
            b.pnr=p[0]; b.name=p[1]; b.age=atoi(p[2].c_str());
            b.trainNo=p[3]; b.trainName=p[4]; b.classType=p[5];
            b.seatNo=atoi(p[6].c_str()); b.fare=atoi(p[7].c_str());
            b.departure=p[8];

            bookings.push_back(b);
        }
        return true;
    }

    bool saveBookings() {
        ofstream f("bookings.csv");
        f<<"pnr,name,age,trainNo,trainName,classType,seatNo,fare,departure\n";

        for (int i=0;i<bookings.size();i++) {
            Booking &b = bookings[i];
            f<<b.pnr<<","<<b.name<<","<<b.age<<","<<b.trainNo<<","
             <<b.trainName<<","<<b.classType<<","<<b.seatNo<<","
             <<b.fare<<","<<b.departure<<"\n";
        }
        return true;
    }

    vector<Train> searchByName(string key) {
        vector<Train> out;
        for (int i=0;i<trains.size();i++)
            if (trains[i].trainName.find(key)!=string::npos)
                out.push_back(trains[i]);
        return out;
    }

    const Train* findTrain(string no) {
        for (int i=0;i<trains.size();i++)
            if (trains[i].trainNo==no) return &trains[i];
        return NULL;
    }

    vector<Booking> findBookings(string pnr) {
        vector<Booking> out;
        for (int i=0;i<bookings.size();i++)
            if (bookings[i].pnr==pnr) out.push_back(bookings[i]);
        return out;
    }

    int booked(string trainNo, string cls) {
        int c=0;
        for (int i=0;i<bookings.size();i++)
            if (bookings[i].trainNo==trainNo && bookings[i].classType==cls)
                c++;
        return c;
    }

    int nextSeat(string trainNo, string cls) {
        return booked(trainNo,cls)+1;
    }

    string makePNR() {
        return to_string(rand()%900000+100000);
    }

    bool addBooking(Booking b) {
        bookings.push_back(b);
        return saveBookings();
    }

    bool cancel(string pnr) {
        for (int i=0;i<bookings.size();i++) {
            if (bookings[i].pnr==pnr) {
                bookings.erase(bookings.begin()+i);
                return saveBookings();
            }
        }
        return false;
    }
};

// ---------------- UI STATE ----------------
int g_page = 1;

char nameBuf[256]="";
char searchBuf[256]="";
char trainNoBuf[256]="";
char ageBuf[16]="";
char pnrBuf[256]="";
char pnrCancelBuf[256]="";

vector<string> trainList;
vector<string> classList;

int selectedTrain = -1;
int selectedClass = -1;

Booking pending;
bool hasPending=false;

// ---------------- Build class list ----------------
void updateClassList(int idx, Database &db) {
    classList.clear();
    selectedClass=-1;
    if (idx<0) return;

    for (set<string>::iterator it=db.trains[idx].classes.begin();
         it!=db.trains[idx].classes.end(); it++)
        classList.push_back(*it);
}

// ---------------- Build train list ----------------
void buildTrainList(Database &db) {
    trainList.clear();
    for (int i=0;i<db.trains.size();i++)
        trainList.push_back(db.trains[i].trainNo+" - "+db.trains[i].trainName);
}

// ---------------- MAIN ----------------
int main() {

    SDL_Init(SDL_INIT_VIDEO);

    SDL_Window* window = SDL_CreateWindow("Train Ticket System",
                    SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                    1280,720, SDL_WINDOW_OPENGL);

    SDL_GLContext gl = SDL_GL_CreateContext(window);
    SDL_GL_SetSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();

    ImGui_ImplSDL2_InitForOpenGL(window,gl);
    ImGui_ImplOpenGL3_Init("#version 130");

    Database db;
    db.loadTrains();
    db.loadBookings();
    buildTrainList(db);

    bool run=true;
    while(run){
        SDL_Event e;
        while(SDL_PollEvent(&e)){
            ImGui_ImplSDL2_ProcessEvent(&e);
            if(e.type==SDL_QUIT) run=false;
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("Main");

        // Sidebar
        ImGui::BeginChild("Side",ImVec2(200,0),true);
        if(ImGui::Button("All Trains",ImVec2(180,30))) g_page=1;
        if(ImGui::Button("Search",ImVec2(180,30))) g_page=2;
        if(ImGui::Button("Availability",ImVec2(180,30))) g_page=3;
        if(ImGui::Button("Book",ImVec2(180,30))) g_page=4;
        if(ImGui::Button("Summary",ImVec2(180,30))) g_page=5;
        if(ImGui::Button("View",ImVec2(180,30))) g_page=6;
        if(ImGui::Button("Cancel",ImVec2(180,30))) g_page=7;
        ImGui::EndChild();

        ImGui::SameLine();

        // Content
        ImGui::BeginChild("Content",ImVec2(0,0),true);

        // 1. All Trains
        if(g_page==1){
            ImGui::Text("All Trains");
            if(ImGui::Button("Reload")) {
                db.loadTrains();
                buildTrainList(db);
            }

            for(int i=0;i<db.trains.size();i++){
                Train&t=db.trains[i];
                ImGui::Text("%s - %s",t.trainNo.c_str(),t.trainName.c_str());
                ImGui::Separator();
            }
        }

        // 2. Search
        if(g_page==2){
            ImGui::InputText("Train Name",searchBuf,256);
            static vector<Train> result;

            if(ImGui::Button("Search"))
                result = db.searchByName(searchBuf);

            for(int i=0;i<result.size();i++){
                ImGui::Text("%s - %s",
                    result[i].trainNo.c_str(),
                    result[i].trainName.c_str());
                ImGui::Separator();
            }
        }

        // 3. Availability
        if(g_page==3){
            ImGui::InputText("Train No",trainNoBuf,256);
            static vector<Train> result;
            if(ImGui::Button("Check")){
                result.clear();
                const Train*t=db.findTrain(trainNoBuf);
                if(t!=NULL) result.push_back(*t);
            }

            if(!result.empty()){
                Train&t=result[0];
                for(set<string>::iterator it=t.classes.begin();it!=t.classes.end();it++){
                    string c=*it;
                    int a=db.seatCapacity[c]-db.booked(t.trainNo,c);
                    ImGui::Text("%s : %d available",c.c_str(),a);
                }
            }
        }

        // 4. Book Ticket
        if(g_page==4){
            ImGui::InputText("Name",nameBuf,256);
            ImGui::InputText("Age",ageBuf,16);

            ImGui::Text("Select Train:");
            for(int i=0;i<trainList.size();i++){
                bool sel = (selectedTrain==i);
                if(ImGui::Selectable(trainList[i].c_str(),sel)){
                    selectedTrain=i;
                    updateClassList(i,db);
                }
            }

            ImGui::Text("Select Class:");
            for(int i=0;i<classList.size();i++){
                bool sel=(selectedClass==i);
                if(ImGui::RadioButton(classList[i].c_str(),sel))
                    selectedClass=i;
            }

            if(ImGui::Button("Proceed")){
                if(selectedTrain>=0 && selectedClass>=0){
                    Train&t=db.trains[selectedTrain];
                    string cls = classList[selectedClass];

                    pending.pnr=db.makePNR();
                    pending.name=nameBuf;
                    pending.age=atoi(ageBuf);
                    pending.trainNo=t.trainNo;
                    pending.trainName=t.trainName;
                    pending.classType=cls;
                    pending.seatNo=db.nextSeat(t.trainNo,cls);
                    pending.fare=db.fares[cls];
                    pending.departure=t.dep;

                    hasPending=true;
                    g_page=5;
                }
            }
        }

        // 5. Summary
        if(g_page==5){
            if(!hasPending) ImGui::Text("No pending booking.");
            else{
                ImGui::Text("Passenger: %s",pending.name.c_str());
                ImGui::Text("Train: %s", pending.trainName.c_str());
                ImGui::Text("Class: %s", pending.classType.c_str());
                ImGui::Text("Fare: %d", pending.fare);

                if(ImGui::Button("Confirm")){
                    db.addBooking(pending);
                    g_page=6;
                }
            }
        }

        // 6. View Ticket
        if(g_page==6){
            ImGui::InputText("PNR",pnrBuf,256);
            static vector<Booking> res;

            if(ImGui::Button("Search"))
                res=db.findBookings(pnrBuf);

            for(int i=0;i<res.size();i++){
                ImGui::Text("Passenger: %s",res[i].name.c_str());
                ImGui::Text("Seat: %d",res[i].seatNo);
                ImGui::Separator();
            }
        }

        // 7. Cancel Ticket
        if(g_page==7){
            ImGui::InputText("PNR",pnrCancelBuf,256);
            static vector<Booking> res;

            if(ImGui::Button("Find"))
                res=db.findBookings(pnrCancelBuf);

            for(int i=0;i<res.size();i++){
                ImGui::Text("Passenger: %s",res[i].name.c_str());
                if(ImGui::Button("Cancel"))
                    db.cancel(res[i].pnr);
                ImGui::Separator();
            }
        }

        ImGui::EndChild();
        ImGui::End();

        // Render
        ImGui::Render();
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(window);
    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
    SDL_GL_DeleteContext(gl);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
