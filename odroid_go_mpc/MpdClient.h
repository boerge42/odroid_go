/*

Uwe Berger; 2019

Basis:
------
https://github.com/kupa79/arduino-mpd-client/blob/master/mpdclient_v0_91.ino
https://www.musicpd.org/doc/html/protocol.html


---------
Have fun!

 */

#ifndef MPDCLIENT_H
#define MPDCLIENT_H

#include <WiFi.h>
#include <odroid_go.h>


#define MAX_RESPONSE_LINES     5   // Bei Bedarf vergrößern
#define MAX_RESPONSE_LINE_SIZE 100 // Bei Bedarf vergrößern  

#define ENTRY_TYP_PLAYLIST    1
#define ENTRY_TYP_DIRECTORY   2
#define ENTRY_TYP_FILE        3

struct playlists_t {
	// char value[MAX_RESPONSE_LINE_SIZE];
	char * value;
  uint8_t entry_typ;
	struct playlists_t *prev;
	struct playlists_t *next;
};



// ***********************************************
class MpdPlaylists{
  private:
    playlists_t *head, *end, *pointer;
  public:
    MpdPlaylists();
    ~MpdPlaylists();
    void insert(uint8_t entry_typ, char* n);
    void clear_list(void);
    bool set_pos_head(void);
    bool set_pos_end(void);
    bool set_pos_next(void);
    bool set_pos_prev(void);
    bool is_empty(void);
    char * get_data(void);
    uint8_t get_data_type(void);
};

// ***********************************************
class MpdResponse{
 public:
 char* errorMsg; 
 char* response[MAX_RESPONSE_LINES][2];
 boolean success;
 
 MpdResponse(){
   success=false;
 }
 ~MpdResponse(){}
 char* getResponseValue(const char* responseKeys);
};

// ***********************************************
class MpdClient {
private:
  MpdResponse mpdResponse;
  MpdPlaylists mpdPlaylists;
  MpdPlaylists mpdDB;

  char mpd_errormsg[MAX_RESPONSE_LINE_SIZE];
  
  WiFiClient client;
  boolean connected;

  MpdResponse* sendCmd(const char* cmd);
  MpdResponse* sendCmd(const char* cmd, const char **params, const char **recognizedResponseKeys);
  boolean contains(const char **recognizedResponseKeys, const char *name);

  void noIdle(void);


public:
  MpdClient();
  ~MpdClient();
  MpdResponse* play();
  MpdResponse* next();
  MpdResponse* prev();
  MpdResponse* pause();
  MpdResponse* stop();
  MpdResponse* random();
  MpdResponse* repeat();
  MpdResponse* currentSong();
  MpdResponse* status();
  MpdResponse* password(const char* password);
  boolean connect(char* serverIp, unsigned int port);
  void disconnect();
  boolean isConnected();

  void startIdle();
  boolean statusChange();

  bool   get_playlists();
  bool   playlists_is_empty(void);
  bool   playlists_set_pos_head(void);
  bool   playlists_set_pos_end(void);
  bool   playlists_set_pos_next(void);
  bool   playlists_set_pos_prev(void);
  char * playlists_get_data(void);

  bool   get_mpd_db(char* db_level);
  bool   mpd_db_is_empty(void);
  bool   mpd_db_set_pos_head(void);
  bool   mpd_db_set_pos_end(void);
  bool   mpd_db_set_pos_next(void);
  bool   mpd_db_set_pos_prev(void);
  char * mpd_db_get_data(void);
  uint8_t mpd_db_get_data_type(void);


  char * last_mpd_errormsg();
  uint8_t status_random;
  uint8_t status_repeat;
  // uint8_t status_pause;

  MpdResponse* add(char* pl);
  MpdResponse* remove(char* pl);

};

#endif




