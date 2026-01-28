#include "httplib.h"
#define WIN32_LEAN_AND_MEAN
#include "json.hpp"
#include <algorithm>
#include <ctime>
#include <iostream>
#include <queue>
#include <string>
#include <vector>
#include <windows.h>

using namespace std;
using json = nlohmann::json;

class IncidentReport {
public:
    int id;
    string cnic;
    string phone;
    string sector;
    string category; 
    string details;
    string currentStatus; 
    time_t recordedAt;

    IncidentReport(int reportId, string idCard, string contact, string loc, string type, string desc)
        : id(reportId), cnic(idCard), phone(contact), sector(loc), category(type), details(desc),
          currentStatus("Pending") {
        recordedAt = time(0);
    }

    json serialize() const {
        return {
            {"id", id}, 
            {"cnic", cnic},
            {"phone", phone}, 
            {"area", sector},
            {"type", category}, 
            {"description", details},
            {"status", currentStatus}, 
            {"timestamp", recordedAt}
        };
    }
};

class ReportNode {
public:
    IncidentReport report;
    ReportNode *next;

    ReportNode(IncidentReport r) : report(r), next(nullptr) {}
};

class ReportRegistry {
private:
    ReportNode *head;
    ReportNode *tail;

public:
    ReportRegistry() : head(nullptr), tail(nullptr) {}

    void registerNew(IncidentReport r) {
        ReportNode *newNode = new ReportNode(r);
        if (!head) {
            head = tail = newNode;
        } else {
            tail->next = newNode;
            tail = newNode;
        }
    }

    ReportNode *fetchHead() { return head; }

    bool finalizeReport(int reportId) {
        ReportNode *current = head;
        while (current) {
            if (current->report.id == reportId) {
                current->report.currentStatus = "Completed";
                return true;
            }
            current = current->next;
        }
        return false;
    }

    json exportAll() {
        json reportsArray = json::array();
        ReportNode *current = head;
        while (current) {
            reportsArray.push_back(current->report.serialize());
            current = current->next;
        }
        return reportsArray;
    }
};

int calculateSeverity(string type) {
    if (type == "Physical Assault") return 3;
    if (type == "Stalking") return 2;
    if (type == "Verbal Abuse") return 1;
    return 0;
}

class SeverityRanker {
public:
    bool operator()(IncidentReport const &r1, IncidentReport const &r2) {
        return calculateSeverity(r1.category) < calculateSeverity(r2.category);
    }
};

class UrgencyQueue {
private:
    priority_queue<IncidentReport, vector<IncidentReport>, SeverityRanker> internalPq;

public:
    void enqueue(IncidentReport r) { internalPq.push(r); }

    json getSortedReports() {
        json sortedList = json::array();
        priority_queue<IncidentReport, vector<IncidentReport>, SeverityRanker> snapshot = internalPq;
        while (!snapshot.empty()) {
            IncidentReport r = snapshot.top();
            snapshot.pop();
            json entry = r.serialize();
            entry["severity_score"] = calculateSeverity(r.category);
            sortedList.push_back(entry);
        }
        return sortedList;
    }
};

class DistrictStats {
public:
    string districtName;
    int incidentCount;
};

class SafetyMonitor {
private:
    vector<DistrictStats> zoneData;
    int riskThreshold;

public:
    SafetyMonitor(int limit) : riskThreshold(limit) {
        vector<string> sectors = {"F-6", "F-7", "F-8", "F-9", "F-10", "F-11", "E-8", "E-9", "E-10", "E-11", "I-8", "I-9", "I-10", "I-11"};
        for (const string &s : sectors) {
            zoneData.push_back({s, 0});
        }
    }

    void logIncident(string area) {
        for (auto &zone : zoneData) {
            if (zone.districtName == area) {
                zone.incidentCount++;
                return;
            }
        }
        zoneData.push_back({area, 1});
    }

    json identifyHotspots() {
        json hotspots = json::array();
        for (const auto &zone : zoneData) {
            if (zone.incidentCount > riskThreshold) {
                hotspots.push_back({{"name", zone.districtName}, {"count", zone.incidentCount}});
            }
        }
        return hotspots;
    }
};

class SafetySystemCore {
private:
    ReportRegistry masterList;
    UrgencyQueue priorityManager;
    SafetyMonitor spatialTracker;
    int totalResolved;
    int idGenerator;

public:
    SafetySystemCore() : spatialTracker(2), totalResolved(0), idGenerator(1) {}

    bool verifyAdmin(string user, string pass) {
        return (user == "admin" && pass == "password123");
    }

    IncidentReport processSubmission(string cnic, string phone, string area, string type, string desc) {
        IncidentReport newEntry(idGenerator++, cnic, phone, area, type, desc);
        masterList.registerNew(newEntry);
        priorityManager.enqueue(newEntry);
        spatialTracker.logIncident(area);
        return newEntry;
    }

    bool closeCase(int id) {
        if (masterList.finalizeReport(id)) {
            totalResolved++;
            return true;
        }
        return false;
    }

    bool modifyStatus(int id, string newStatus) {
        ReportNode *current = masterList.fetchHead();
        while (current) {
            if (current->report.id == id) {
                current->report.currentStatus = newStatus;
                if (newStatus == "Completed") totalResolved++;
                return true;
            }
            current = current->next;
        }
        return false;
    }

    json fetchIntelligence() {
        return {
            {"completed_count", totalResolved},
            {"total_complaints", idGenerator - 1},
            {"red_zones", spatialTracker.identifyHotspots()},
            {"priority_queue", priorityManager.getSortedReports()},
            {"all_complaints", masterList.exportAll()}
        };
    }
};

void applyCors(httplib::Response &res) {
    res.set_header("Access-Control-Allow-Origin", "*");
    res.set_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    res.set_header("Access-Control-Allow-Headers", "Content-Type");
}

int main() {
    SafetySystemCore app;
    
    app.processSubmission("12345-1234567-1", "03001234567", "F-6", "Verbal Abuse", "Catcalling near park");
    app.processSubmission("54321-7654321-2", "03337654321", "F-10", "Physical Assault", "Attacked in street");
    app.processSubmission("11111-1111111-1", "03211111111", "F-6", "Stalking", "Followed home");
    app.processSubmission("22222-2222222-2", "03452222222", "F-6", "Verbal Abuse", "Harassed at bus stop");

    httplib::Server server;

    server.Options("/(.*)", [](const httplib::Request &, httplib::Response &res) {
        applyCors(res);
        res.status = 204;
    });

    server.Post("/api/submit", [&](const httplib::Request &req, httplib::Response &res) {
        applyCors(res);
        try {
            auto payload = json::parse(req.body);
            auto report = app.processSubmission(payload["cnic"], payload["phone"], payload["area"], payload["type"], payload["description"]);
            res.set_content(json({{"status", "success"}, {"message", "Submission received"}, {"complaint", report.serialize()}}).dump(), "application/json");
        } catch (const std::exception &e) {
            res.status = 400;
            res.set_content(json({{"status", "error"}, {"message", e.what()}}).dump(), "application/json");
        }
    });

    server.Post("/api/admin/login", [&](const httplib::Request &req, httplib::Response &res) {
        applyCors(res);
        try {
            auto payload = json::parse(req.body);
            if (app.verifyAdmin(payload["username"], payload["password"])) {
                res.set_content(json({{"status", "success"}}).dump(), "application/json");
            } else {
                res.status = 401;
                res.set_content(json({{"status", "error"}, {"message", "Auth failed"}}).dump(), "application/json");
            }
        } catch (...) { res.status = 400; }
    });

    server.Get("/api/dashboard", [&](const httplib::Request &, httplib::Response &res) {
        applyCors(res);
        res.set_content(app.fetchIntelligence().dump(), "application/json");
    });

    server.Post("/api/complete", [&](const httplib::Request &req, httplib::Response &res) {
        applyCors(res);
        try {
            auto payload = json::parse(req.body);
            if (app.closeCase(payload["id"])) {
                res.set_content(json({{"status", "success"}}).dump(), "application/json");
            } else {
                res.status = 404;
                res.set_content(json({{"status", "error"}, {"message", "Not found"}}).dump(), "application/json");
            }
        } catch (...) { res.status = 400; }
    });

    server.Post("/api/update-status", [&](const httplib::Request &req, httplib::Response &res) {
        applyCors(res);
        try {
            auto payload = json::parse(req.body);
            if (app.modifyStatus(payload["id"], payload["status"])) {
                res.set_content(json({{"status", "success"}}).dump(), "application/json");
            } else {
                res.status = 404;
                res.set_content(json({{"status", "error"}, {"message", "Not found"}}).dump(), "application/json");
            }
        } catch (...) { res.status = 400; }
    });

    cout << "Internal Service active on port 8080..." << endl;
    server.listen("0.0.0.0", 8080);
    return 0;
}