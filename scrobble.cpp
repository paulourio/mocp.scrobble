/* Copyright (C) 2011 Paulo Roberto Urio <paulourio@gmail.com>
 * 
 */
#include <iostream>
#include <string>
#include <getopt.h>
#include <cstdlib>
#include <lastfmscrobbler.h>

using namespace std;

namespace song {
    char *user, *pass, *artist, *album, *title;
    int track, duration;
}

class Scrobble {
  public:
    Scrobble();
  private:
    void start();
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
            {0, 0, 0, 0}
        };
    while ((c = getopt_long(argc, argv, "h:u:p:a:b:t:n:d:", long_options, NULL)) != -1) {
        switch (c) {
            case 'h': cout << "help" << endl; break;            
            case 'u': song::user = optarg; count++; break;
            case 'p': song::pass = optarg; count++; break;
            case 'a': song::artist = optarg; count++; break;
            case 'b': song::album = optarg; count++; break;
            case 't': song::title = optarg; count++; break;
            case 'n': song::track = atoi(optarg); count++; break;
            case 'd': song::duration = atoi(optarg); count++; break; 
        }
    }
    if (help == 1) {
        usage();
        exit(0);
    }
    if (count != 7) {
        cout << "I accept only 7 options. See --help for usage." << endl;
        exit(1);
    }
    Scrobble();
    return 0;
}
