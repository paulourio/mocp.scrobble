/* Copyright (C) 2011 Paulo Roberto Urio <paulourio@gmail.com>
 * 
 * This work is licensed under the Creative Commons Attribution-NonCommercial 
 * 3.0 Unported License. To view a copy of this license, visit 
 * http://creativecommons.org/licenses/by-nc/3.0/ or send a letter to Creative 
 * Commons, 444 Castro Street, Suite 900, Mountain View, California, 94041, USA.
 */
#include <iostream>
#include <sstream>
#include <string>
#include <list>
#include <getopt.h>
#include <stdio.h>
#include <cstdlib>
#include <unistd.h>
#include <lastfmscrobbler.h>

using namespace std;

namespace song {
    char *user, *pass, *artist, *album, *title, *file;
    int track, duration;
}

class Scrobble {
  public:
    Scrobble();
  private:
    void start();
    std::string exec(std::string cmd);
    int checkStatus();
    
    bool paused;
};

Scrobble::Scrobble() {
    this->start();
}

void Scrobble::start() {
    cout << "Logging with " << song::user << endl;
    LastFmScrobbler scrobbler(song::user, song::pass, false, true);
    scrobbler.authenticate();
        
    SubmissionInfo info(song::artist, song::title);
    info.setAlbum(song::album);
    info.setTrackNr(song::track);
    info.setTrackLength(song::duration);

    scrobbler.startedPlaying(info);
    paused = false;
    
    int status = 1;
    
    while (status != 0) {
        status = checkStatus();
        
        switch (status) {
            case 0:
                cout << "Something went wrong, exiting." << endl;
                exit(1);
            case 1:
                scrobbler.pausePlaying(paused); // Update paused information.
                break;
            case 2:
                scrobbler.finishedPlaying();
                exit(0);
        }   
        usleep(5000000); 
    }
}

int Scrobble::checkStatus() {
    std::string cmd = exec("/usr/bin/which mocp");
    cmd.erase(cmd.length() - 1, 1);
    cmd.append(" -Q %file\\\\n%state\\\\n%cs");
    cout << "Running " << cmd << endl;
    
    /* Start of MOCP wrapper */
    FILE* pipe = popen((char *) cmd.c_str(), "r");
    if (!pipe) {
        cout << "No output pipe" << endl;
        return 0;
    }
    char buffer[128];
    list<string> result;
    
    while (!feof(pipe))
    	if (fgets(buffer, 128, pipe) != NULL)
    		result.push_back( buffer );
    int stat_info = pclose(pipe);
    int status_code = WEXITSTATUS(stat_info);
    
    if (status_code == 2) {
        cout << "MOCP not running." << endl;
        return 0;
    }
    
    #define POP(x)  x.front(); x.pop_front()
    if (result.size() == 3) {
        std::string cfile = POP(result); // Which file is playing
        cfile.erase(cfile.length() - 1, 1);
        std::string cstate = POP(result); // Player state
        cstate.erase(cstate.length() - 1, 1);
        std::string csec = POP(result); // Current seconds
        csec.erase(csec.length() - 1, 1);
        
        if (cfile != song::file) {
            cout << cfile << endl;
            cout << song::file << endl;
            cout << "Not playing this song anymore." << endl;
            return 0;
        }
        if (cstate == "STOP") {
            cout << "Player stopped." << endl;
            return 0;
        }
        if (cstate == "PLAY") {
            paused = false;
        } else
        if (cstate == "PAUSE") {
            paused = true;
        }
        int halftime = song::duration / 2;
        if (atoi(csec.c_str()) > halftime) {
            cout << "Marking song as listened." << endl;
            return 2;
        }
    } else {
        cout << "Unexpected syntax!" << endl;
        return 0;
    }
    
    return 1;
}

std::string Scrobble::exec(std::string cmd) {
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) return "ERROR";
    char buffer[128];
    std::stringstream result;
    while (!feof(pipe))
    	if (fgets(buffer, 128, pipe) != NULL)
    		result << buffer;
    pclose(pipe);
    return result.str();
}

void usage() {
    cout << "./scrobble [options]" << endl;    
    cout << "\t-h   --help      This message." << endl;
    cout << "\t-u   --user      Last.fm username." << endl;
    cout << "\t-p   --pass      Last.fm password. Note: plaintext." << endl;
    cout << "\t-a   --artist    Artist name." << endl;
    cout << "\t-b   --album     Album name." << endl;
    cout << "\t-t   --title     Song title." << endl;
    cout << "\t-n   --track     Album track number of song." << endl;
    cout << "\t-d   --duration  Song duration in seconds." << endl;
}

int main(int argc, char* argv[]) {
    int c;
    int help = 0, count = 0;

    struct option long_options[] =
        {
            {"help", no_argument, &help, 1},
            {"user",  required_argument, 0, 'u'},
            {"pass",  required_argument, 0, 'p'},
            {"artist",  required_argument, 0, 'a'},
            {"album",  required_argument, 0, 'b'},
            {"title",  required_argument, 0, 't'},
            {"track",  required_argument, 0, 'n'},
            {"duration",  required_argument, 0, 'd'},
            {"file",  required_argument, 0, 'f'},
            {0, 0, 0, 0}
        };
    while ((c = getopt_long(argc, argv, "h:u:p:a:b:t:n:d:f:", long_options, NULL)) != -1) {
        switch (c) {
            case 'h': cout << "help" << endl; break;            
            case 'u': song::user = optarg; count++; break;
            case 'p': song::pass = optarg; count++; break;
            case 'a': song::artist = optarg; count++; break;
            case 'b': song::album = optarg; count++; break;
            case 't': song::title = optarg; count++; break;
            case 'n': song::track = atoi(optarg); count++; break;
            case 'd': song::duration = atoi(optarg); count++; break; 
            case 'f': song::file = optarg; count++; break; 
        }
    }
    if (help == 1) {
        usage();
        exit(0);
    }
    if (count != 8) {
        cout << "I'm only accepting 8 options. See --help for usage." << endl;
        exit(1);
    }
    Scrobble();
    return 0;
}
