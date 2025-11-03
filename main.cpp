#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <sstream>

using namespace std;

struct Submission {
    string problem;
    string status;
    int time;
    bool isFrozen;

    Submission(string p, string s, int t, bool f = false) : problem(p), status(s), time(t), isFrozen(f) {}
};

struct ProblemStatus {
    bool solved;
    int wrongAttempts;
    int solveTime;
    int frozenSubmissions;
    int wrongBeforeFreeze;
    vector<Submission> frozenSubs;

    ProblemStatus() : solved(false), wrongAttempts(0), solveTime(0),
                      frozenSubmissions(0), wrongBeforeFreeze(0) {}
};

struct Team {
    string name;
    map<string, ProblemStatus> problems;
    vector<Submission> submissions;
    int solvedCount;
    int penaltyTime;
    vector<int> solveTimes;
    
    Team(string n) : name(n), solvedCount(0), penaltyTime(0) {}
    
    void addSubmission(const string& problem, const string& status, int time, bool frozen) {
        submissions.push_back(Submission(problem, status, time, frozen));

        if (!problems[problem].solved) {
            if (status == "Accepted") {
                if (!frozen) {
                    problems[problem].solved = true;
                    problems[problem].solveTime = time;
                    solvedCount++;
                    penaltyTime += 20 * problems[problem].wrongAttempts + time;
                    solveTimes.push_back(time);
                }
            } else {
                if (!frozen) {
                    problems[problem].wrongAttempts++;
                }
            }
        }

        if (frozen && !problems[problem].solved) {
            problems[problem].frozenSubmissions++;
            problems[problem].frozenSubs.push_back(Submission(problem, status, time, frozen));
        }
    }
};

bool compareTeams(Team* a, Team* b) {
    if (a->solvedCount != b->solvedCount) {
        return a->solvedCount > b->solvedCount;
    }
    if (a->penaltyTime != b->penaltyTime) {
        return a->penaltyTime < b->penaltyTime;
    }
    // Sort solve times if needed
    if (!is_sorted(a->solveTimes.rbegin(), a->solveTimes.rend())) {
        sort(a->solveTimes.rbegin(), a->solveTimes.rend());
    }
    if (!is_sorted(b->solveTimes.rbegin(), b->solveTimes.rend())) {
        sort(b->solveTimes.rbegin(), b->solveTimes.rend());
    }
    int minSize = min(a->solveTimes.size(), b->solveTimes.size());
    for (int i = 0; i < minSize; i++) {
        if (a->solveTimes[i] != b->solveTimes[i]) {
            return a->solveTimes[i] < b->solveTimes[i];
        }
    }
    return a->name < b->name;
}

class ICPCSystem {
private:
    map<string, Team*> teams;
    vector<Team*> teamList;
    bool started;
    bool frozen;
    bool flushed;
    int durationTime;
    int problemCount;
    int freezeTime;
    vector<string> problemNames;

public:
    ICPCSystem() : started(false), frozen(false), flushed(false), durationTime(0), problemCount(0), freezeTime(-1) {}
    
    void addTeam(const string& name) {
        if (started) {
            cout << "[Error]Add failed: competition has started.\n";
        } else if (teams.find(name) != teams.end()) {
            cout << "[Error]Add failed: duplicated team name.\n";
        } else {
            Team* team = new Team(name);
            teams[name] = team;
            teamList.push_back(team);
            cout << "[Info]Add successfully.\n";
        }
    }
    
    void start(int duration, int problems) {
        if (started) {
            cout << "[Error]Start failed: competition has started.\n";
        } else {
            started = true;
            durationTime = duration;
            problemCount = problems;
            for (int i = 0; i < problems; i++) {
                problemNames.push_back(string(1, 'A' + i));
            }
            // Sort teams by name initially
            sort(teamList.begin(), teamList.end(), [](const Team* a, const Team* b) {
                return a->name < b->name;
            });
            cout << "[Info]Competition starts.\n";
        }
    }
    
    void submit(const string& problem, const string& teamName, const string& status, int time) {
        Team* team = teams[teamName];
        team->addSubmission(problem, status, time, frozen);
    }
    
    void flush() {
        flushed = true;
        sort(teamList.begin(), teamList.end(), compareTeams);
        cout << "[Info]Flush scoreboard.\n";
    }
    
    void freeze() {
        if (frozen) {
            cout << "[Error]Freeze failed: scoreboard has been frozen.\n";
        } else {
            frozen = true;
            for (auto& team : teamList) {
                for (auto& p : team->problems) {
                    if (!p.second.solved) {
                        p.second.wrongBeforeFreeze = p.second.wrongAttempts;
                    }
                }
            }
            cout << "[Info]Freeze scoreboard.\n";
        }
    }
    
    void scroll() {
        if (!frozen) {
            cout << "[Error]Scroll failed: scoreboard has not been frozen.\n";
            return;
        }

        cout << "[Info]Scroll scoreboard.\n";
        sort(teamList.begin(), teamList.end(), compareTeams);
        printScoreboard();
        
        // Unfreeze problems one by one
        while (true) {
            // Find lowest ranked team with frozen problems
            Team* targetTeam = nullptr;
            string targetProblem = "";

            for (int i = teamList.size() - 1; i >= 0; i--) {
                Team* team = teamList[i];
                for (const auto& pname : problemNames) {
                    if (team->problems[pname].frozenSubmissions > 0) {
                        targetTeam = team;
                        targetProblem = pname;
                        break;
                    }
                }
                if (targetTeam) break;
            }

            if (!targetTeam) break;
            
            // Unfreeze this problem
            int oldRank = -1;
            for (int i = 0; i < teamList.size(); i++) {
                if (teamList[i] == targetTeam) {
                    oldRank = i;
                    break;
                }
            }
            
            // Process frozen submissions
            int frozenWrongCount = 0;
            bool problemSolved = false;
            bool wasSolved = targetTeam->problems[targetProblem].solved;

            if (!wasSolved) {
                for (const auto& sub : targetTeam->problems[targetProblem].frozenSubs) {
                    if (sub.status == "Accepted") {
                        targetTeam->problems[targetProblem].solved = true;
                        targetTeam->problems[targetProblem].solveTime = sub.time;
                        targetTeam->problems[targetProblem].wrongAttempts = targetTeam->problems[targetProblem].wrongBeforeFreeze + frozenWrongCount;
                        targetTeam->solvedCount++;
                        targetTeam->penaltyTime += 20 * (targetTeam->problems[targetProblem].wrongBeforeFreeze + frozenWrongCount) + sub.time;
                        targetTeam->solveTimes.push_back(sub.time);
                        problemSolved = true;
                        break;
                    } else {
                        frozenWrongCount++;
                    }
                }
            }
            // Update wrongAttempts to include frozen wrong attempts (only if not solved)
            if (!problemSolved) {
                targetTeam->problems[targetProblem].wrongAttempts = targetTeam->problems[targetProblem].wrongBeforeFreeze + frozenWrongCount;
            }
            targetTeam->problems[targetProblem].frozenSubmissions = 0;
            targetTeam->problems[targetProblem].frozenSubs.clear();
            
            // Re-sort and check if ranking changed
            sort(teamList.begin(), teamList.end(), compareTeams);
            
            int newRank = -1;
            for (int i = 0; i < teamList.size(); i++) {
                if (teamList[i] == targetTeam) {
                    newRank = i;
                    break;
                }
            }
            
            if (newRank < oldRank) {
                cout << targetTeam->name << " " << teamList[newRank + 1]->name << " "
                     << targetTeam->solvedCount << " " << targetTeam->penaltyTime << "\n";
            }
        }
        
        printScoreboard();
        frozen = false;
    }
    
    void printScoreboard() {
        for (int i = 0; i < teamList.size(); i++) {
            Team* team = teamList[i];
            cout << team->name << " " << (i + 1) << " " << team->solvedCount << " " << team->penaltyTime;
            
            for (const auto& pname : problemNames) {
                cout << " ";
                const ProblemStatus& ps = team->problems[pname];
                if (ps.frozenSubmissions > 0) {
                    if (ps.wrongBeforeFreeze > 0) {
                        cout << "-" << ps.wrongBeforeFreeze << "/" << ps.frozenSubmissions;
                    } else {
                        cout << "0/" << ps.frozenSubmissions;
                    }
                } else if (ps.solved) {
                    if (ps.wrongAttempts > 0) {
                        cout << "+" << ps.wrongAttempts;
                    } else {
                        cout << "+";
                    }
                } else {
                    if (ps.wrongAttempts > 0) {
                        cout << "-" << ps.wrongAttempts;
                    } else {
                        cout << ".";
                    }
                }
            }
            cout << "\n";
        }
    }
    
    void queryRanking(const string& teamName) {
        if (teams.find(teamName) == teams.end()) {
            cout << "[Error]Query ranking failed: cannot find the team.\n";
            return;
        }

        cout << "[Info]Complete query ranking.\n";
        if (frozen) {
            cout << "[Warning]Scoreboard is frozen. The ranking may be inaccurate until it were scrolled.\n";
        }

        // If not flushed yet, use lexicographic order
        if (!flushed) {
            int rank = 1;
            for (const auto& team : teamList) {
                if (team->name == teamName) {
                    break;
                }
                rank++;
            }
            cout << teamName << " NOW AT RANKING " << rank << "\n";
        } else {
            int rank = -1;
            for (int i = 0; i < teamList.size(); i++) {
                if (teamList[i]->name == teamName) {
                    rank = i + 1;
                    break;
                }
            }
            cout << teamName << " NOW AT RANKING " << rank << "\n";
        }
    }
    
    void querySubmission(const string& teamName, const string& problem, const string& status) {
        if (teams.find(teamName) == teams.end()) {
            cout << "[Error]Query submission failed: cannot find the team.\n";
            return;
        }
        
        cout << "[Info]Complete query submission.\n";
        
        Team* team = teams[teamName];
        Submission* found = nullptr;
        
        for (int i = team->submissions.size() - 1; i >= 0; i--) {
            const Submission& sub = team->submissions[i];
            bool matchProblem = (problem == "ALL" || sub.problem == problem);
            bool matchStatus = (status == "ALL" || sub.status == status);
            
            if (matchProblem && matchStatus) {
                found = &team->submissions[i];
                break;
            }
        }
        
        if (found) {
            cout << teamName << " " << found->problem << " " << found->status << " " << found->time << "\n";
        } else {
            cout << "Cannot find any submission.\n";
        }
    }
    
    void end() {
        cout << "[Info]Competition ends.\n";
    }
};

int main() {
    ICPCSystem system;
    string line;
    
    while (getline(cin, line)) {
        istringstream iss(line);
        string cmd;
        iss >> cmd;
        
        if (cmd == "ADDTEAM") {
            string name;
            iss >> name;
            system.addTeam(name);
        } else if (cmd == "START") {
            string dummy;
            int duration, problems;
            iss >> dummy >> duration >> dummy >> problems;
            system.start(duration, problems);
        } else if (cmd == "SUBMIT") {
            string problem, by, teamName, with, status, at;
            int time;
            iss >> problem >> by >> teamName >> with >> status >> at >> time;
            system.submit(problem, teamName, status, time);
        } else if (cmd == "FLUSH") {
            system.flush();
        } else if (cmd == "FREEZE") {
            system.freeze();
        } else if (cmd == "SCROLL") {
            system.scroll();
        } else if (cmd == "QUERY_RANKING") {
            string teamName;
            iss >> teamName;
            system.queryRanking(teamName);
        } else if (cmd == "QUERY_SUBMISSION") {
            string teamName, where, rest;
            iss >> teamName >> where;
            getline(iss, rest);
            
            size_t problemPos = rest.find("PROBLEM=");
            size_t statusPos = rest.find("STATUS=");
            
            string problem = rest.substr(problemPos + 8, statusPos - problemPos - 13);
            string status = rest.substr(statusPos + 7);
            
            system.querySubmission(teamName, problem, status);
        } else if (cmd == "END") {
            system.end();
            break;
        }
    }
    
    return 0;
}

