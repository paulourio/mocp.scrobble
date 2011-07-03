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
#include <string.h>
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
    std::string exec(std::string cmd, bool removeNewLine);
    int checkStatus();
    
    bool paused;

    static const int EVENT_PAUSE = 0;
    static const int EVENT_STOP = 1;
    static const int EVENT_RESUME = 2;
    static const int EVENT_COMPLETE = 3;
};

Scrobble::Scrobble() {
    this->start();
}

void Scrobble::start() {
    cout << "Logging with " << song::user << endl;
    LastFmScrobbler scrobbler(song::user, song::pass, false, false);
    scrobbler.authenticate();
        
    SubmissionInfo info(song::artist, song::title);
    info.setAlbum(song::album);
    info.setTrackNr(song::track);
    info.setTrackLength(song::duration / 2);

    scrobbler.startedPlaying(info);
    paused = false;
    
    int status = 1;
    
    while (status != 0) {
        status = checkStatus();
        
        switch (status) {
            case EVENT_STOP: // Stop scrobble due fail or cancel
                cout << "Something went wrong, exiting." << endl;
                scrobbler.finishedPlaying();
                exit(1);
            case EVENT_RESUME:
                break;
            case EVENT_PAUSE:
                scrobbler.pausePlaying(paused); // Update paused information.    
                break;
            case EVENT_COMPLETE:
                exit(0);
                break;
        }   
        usleep(5000000); 
    }
}

int Scrobble::checkStatus() {
    std::string cmd = exec("/usr/bin/which mocp", true);
    cmd.append(" -Q %file\\\\n%state\\\\n%cs");
    cout << "Running " << cmd << endl;
    
    /* Start of MOCP wrapper */
    FILE* pipe = popen((char *) cmd.c_str(), "r");
    if (!pipe) {
        cout << "No output pipe" << endl;
        return EVENT_STOP;
    }
    char buffer[128];
    list<string> result;
    
    while (!feof(pipe))
        if (fgets(buffer, 128, pipe) != NULL) {
            char    *end = buffer + strlen(buffer) - 1;
            if (*end == '\n')
                *end = 0;
            result.push_back( buffer );
        }
    int stat_info = pclose(pipe);
    int status_code = WEXITSTATUS(stat_info);
    
    if (status_code == 2) {
        cout << "MOCP not running." << endl;
        return EVENT_STOP;
    }
    
    #define POP(x)  x.front(); x.pop_front()
    if (result.size() == 3) {
        std::string cfile = POP(result); // Which file is playing
        std::string cstate = POP(result); // Player state
        cstate.erase(cstate.length() - 1, 1);
        std::string csec = POP(result); // Current seconds
        csec.erase(csec.length() - 1, 1);
        
        if (cfile != song::file) {
            cout << cfile << endl;
            cout << song::file << endl;
            cout << "Not playing this song anymore." << endl;
            return EVENT_STOP;
        }
        if (cstate == "STOP") {
            cout << "Player stopped." << endl;
            return EVENT_STOP;
        }
        if (cstate == "PLAY" && paused == true) {
            paused = false;
            return EVENT_PAUSE;
        } else
        if (cstate == "PAUSE" && paused == false) {
            paused = true;
            return EVENT_PAUSE;
        }
        int halftime = song::duration / 2;
        if (atoi(csec.c_str()) > halftime) {
            cout << "Marking song as listened." << endl;
            return EVENT_COMPLETE;
        }
    } else {
        cout << "Unexpected syntax!" << endl;
        return EVENT_STOP;
    }
    
    return EVENT_RESUME;
}

std::string Scrobble::exec(std::string cmd, bool removeNewline) {
    FILE    *pipe = popen(cmd.c_str(), "r");

    if (!pipe) 
        return "ERROR";
    char    buffer[128];

    std::stringstream result;
    while (!feof(pipe))
        if (fgets(buffer, 128, pipe) != NULL) {
            if (removeNewline != false && buffer[strlen(buffer)-1] == '\n')
                buffer[strlen(buffer) - 1] = 0;
            result << buffer;
        }
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

int main(int argc, char** argv) {
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
    char    *opts = "h:u:p:a:b:t:n:d:f:";
    while ((c = getopt_long(argc, argv, opts, long_options, NULL)) != -1) {
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
