/*

Uwe Berger; 2019

Basis:
------
https://github.com/kupa79/arduino-mpd-client/blob/master/mpdclient_v0_91.ino
https://www.musicpd.org/doc/html/protocol.html


---------
Have fun!

 */
#include "MpdClient.h"

// ***********************************************
// Helferlein
// ***********************************************

// ***********************************************
void trim(char * s) {
    char * p = s;
    int l = strlen(p);

    while(isspace(p[l - 1])) p[--l] = 0;
    while(* p && isspace(* p)) ++p, --l;

    memmove(s, p, l + 1);
}


// ***********************************************
MpdClient::MpdClient(){
  this->connected = false;
  
  mpdResponse.errorMsg=(char*)realloc(mpdResponse.errorMsg, sizeof(char));
  mpdResponse.errorMsg[0]='\0';
  
  for(int i=0;i<MAX_RESPONSE_LINES;i++){
    mpdResponse.response[i][0]=(char*)realloc(mpdResponse.response[i][0], sizeof(char));
    mpdResponse.response[i][1]=(char*)realloc(mpdResponse.response[i][1], sizeof(char));
    mpdResponse.response[i][0][0]='\0';
    mpdResponse.response[i][1][0]='\0';
  }
}

// ***********************************************
MpdClient::~MpdClient(){
  
  free(mpdResponse.errorMsg);
  
  for(int i=0;i<MAX_RESPONSE_LINES;i++){
    free(mpdResponse.response[i][0]);
    free(mpdResponse.response[i][1]);
  }
}

// ***********************************************
MpdResponse* MpdClient::play(){
  return sendCmd("play");
}

// ***********************************************
MpdResponse* MpdClient::next(){
  return sendCmd("next");
}

// ***********************************************
MpdResponse* MpdClient::prev(){
  return sendCmd("previous");
}

// ***********************************************
MpdResponse* MpdClient::pause(){
  return sendCmd("pause");
  // char buf[10];
  // sprintf(buf, "plause %i", !status_pause);
  // return sendCmd(buf);
}

// ***********************************************
MpdResponse* MpdClient::stop(){
  return sendCmd("stop");
}

// ***********************************************
MpdResponse* MpdClient::random(){
  char buf[10];
  sprintf(buf, "random %i", !status_random);
  return sendCmd(buf);
}

// ***********************************************
MpdResponse* MpdClient::repeat(){
  char buf[10];
  sprintf(buf, "repeat %i", !status_repeat);
  return sendCmd(buf);
}

// ***********************************************
MpdResponse* MpdClient::currentSong(){
  const char* recognized[] = {
    "Artist","Title", "Album", "Date", "Name", NULL};
  return sendCmd("currentsong", NULL, recognized);
}

// ***********************************************
MpdResponse* MpdClient::password(const char *password){
  const char* params[] = {
    password, NULL      };
  return sendCmd("password", params, NULL);
}

// ***********************************************
MpdResponse* MpdClient::status(){
  const char* recognized[] = {
    "state", "repeat", "random",NULL      };
  MpdResponse * r;
  r = sendCmd("status", NULL, recognized);
  // ein paar Status rausfischen
  status_repeat=atoi(r->getResponseValue("repeat"));
  status_random=atoi(r->getResponseValue("random"));
  // if (!strcmp(r->getResponseValue("state"), "pause")) {
  //   status_pause = 1;
  // } else {
  //   status_pause = 0;
  // }
  return r;
}

// ***********************************************
MpdResponse* MpdClient::add(char * pl){
  char buf[200];
  sprintf(buf, "load %s", pl);
  return sendCmd(buf);
  // return sendCmd("stop");
}

// ***********************************************
MpdResponse* MpdClient::remove(char * pl){
  char buf[200];
  // return sendCmd("stop");
  sendCmd("clear");
  sprintf(buf, "load %s", pl);
  sendCmd(buf);
  sendCmd("play");
}




// ***********************************************
char* MpdResponse::getResponseValue(const char * name){
  for(int i=0;i<MAX_RESPONSE_LINES;i++){
    if(strcmp(response[i][0], name) == 0){
      return response[i][1];
    }
  }
  return NULL; 
}

// ***********************************************
boolean MpdClient::contains(const char **recognized, const char *name){
  for(int i=0;recognized!=NULL && i<MAX_RESPONSE_LINES;i++){
    if(recognized[i]!=NULL){
      if(strcmp(name, recognized[i]) == 0){
        return true;
      }
    }
    else{
      break;
    }
  }
  return false;  
}

// ***********************************************
MpdResponse* MpdClient::sendCmd(const char* cmd){
  return sendCmd(cmd, NULL, NULL);
}

// ***********************************************
MpdResponse* MpdClient::sendCmd(const char* cmd, const char **params, const char **recognized){
 
  // zuerst MPD-idle ausschalten, sonst wird kein Kommando angenommen
  noIdle();
  
  mpdResponse.errorMsg[0]='\0';
  
  for(int i=0;i<MAX_RESPONSE_LINES;i++){
    mpdResponse.response[i][0][0]='\0';
    mpdResponse.response[i][1][0]='\0';
  }

  client.print(cmd);
  for(int a=0;a<3;a++){
    if(params!=NULL && params[a]!=NULL){
      client.print(" ");
      client.print(params[a]);
    }
    else{
      break;
    }
  }
  client.println();

  while(!client.available()){
    //debug("Waiting for response...");
    if(!isConnected()){
      mpdResponse.success = false;
      return &mpdResponse;
    }
  }

  unsigned int responseIndex=0;
  char * line = NULL; 
  unsigned int lineIndex=0;
  
  while(client.available())  {
    char c = client.read();
    if(c != '\n'){
      if(lineIndex<MAX_RESPONSE_LINE_SIZE-1){
        line = (char*) realloc(line, (lineIndex+2) * sizeof(char));
        line[lineIndex]=c;
        line[lineIndex+1]='\0';
        lineIndex++;
      }
    }
    else{
      if(strncmp(line, "OK", 2)==0){
        free(line);
        mpdResponse.success = true;
        // idle wieder einschalten
        startIdle();
        return &mpdResponse;
      }
      else if(strncmp(line, "ACK", 3)==0){
        char* msg = line + 4;
        mpdResponse.errorMsg = (char*) realloc(mpdResponse.errorMsg, strlen(msg) + 1);
        strcpy(mpdResponse.errorMsg, msg);
        free(line);
        mpdResponse.success = false;
        // idle wieder einschalten
        startIdle();
        return &mpdResponse;
      }

      for(int i=0; i<lineIndex; i++){
        if(line[i] == ':'){
          line[i] = '\0';
          char* name = line;
          char* val = line + i + 2;
          if(contains(recognized, name) && responseIndex<MAX_RESPONSE_LINES){
            mpdResponse.response[responseIndex][0]=(char*)realloc(mpdResponse.response[responseIndex][0], (strlen(name)+1)*sizeof(char));
            mpdResponse.response[responseIndex][1]=(char*)realloc(mpdResponse.response[responseIndex][1], (strlen(val)+1)*sizeof(char));
            trim(name);
            trim(val);
            strncpy(mpdResponse.response[responseIndex][0], name, strlen(name));
            strncpy(mpdResponse.response[responseIndex][1], val, strlen(val));
            mpdResponse.response[responseIndex][0][strlen(name)]='\0';
            mpdResponse.response[responseIndex][1][strlen(val)]='\0';
            responseIndex++;
          }
          break;
        }
      }
      lineIndex=0;
      line[0] = '\0';
    }
  }
  free(line);
  // idle wieder einschalten
  startIdle();
}

// ***********************************************
boolean MpdClient::isConnected(){
  connected = connected && client.connected();
  return connected;
}

// ***********************************************
void MpdClient::disconnect(){
  client.stop();
  connected = false;
}

// ***********************************************
boolean MpdClient::connect(char* serverIp, unsigned int port){
  if(!isConnected() && client.connect(serverIp, port)){
    char connectResponse[20]="";
    while(!client.available());
    unsigned int i=0;
    while(client.available()){
      connectResponse[i] = (char) client.read();
      connectResponse[i+1] = '\0';
      i++;
    }
    if(strncmp(connectResponse, "OK MPD", 6) == 0){
      connected = true;
    }
  }
  return connected;
}

// ***********************************************
void MpdClient::noIdle(){
  // einfach nur "noidle" senden, Return abwarten/auslesen, 
  // aber (erst mal) keine Fehlerbehandlung
  client.println("noidle");
  while(!client.available());
  while(client.available())  {
      char c = client.read();
  }
}

// ***********************************************
void MpdClient::startIdle(){
  client.println("idle");
}

// ***********************************************
boolean MpdClient::statusChange(){
  // ist was vom MPD gesendet worden?
  if (client.available()){
    // einfach nur einlesen, nichts weiter auswerten
    while(client.available())  {
      char c = client.read();
    }
    // Client wieder lauschen lassen
    client.println("idle");
    return true;
  }
  return false;
}

// ***********************************************
char * MpdClient::last_mpd_errormsg() {
  return mpd_errormsg;
}

// ***********************************************
bool MpdClient::get_playlists() {
  char c;
  char buf[MAX_RESPONSE_LINE_SIZE];
  unsigned int i=0;
  char * pch;
  // alte Liste loeschen
  mpdPlaylists.clear_list();
  // zuerst MPD-idle ausschalten, sonst wird kein Kommando angenommen
  noIdle();
  // Kommando senden
  client.println("listplaylists");
  // warten bis etwas kommt
  while(!client.available());
  // Einlesen und verarbeiten
  while(client.available())  {
    char c = client.read();
    if (c != '\n') {
      if (i < MAX_RESPONSE_LINE_SIZE-1) {
        buf[i] = c;
        buf[i+1] = 0;
        i++;
      }
    } else {
      if(strncmp(buf, "OK", 2)==0){
        // fertig und alles gut
        startIdle();
        //return playlist;
        return true;
      }
      else if (strncmp(buf, "ACK", 3)==0) {
        // ...bis mir besseres einfaellt :-)
        startIdle();
        //return playlist;
        strncpy(mpd_errormsg, buf, sizeof(mpd_errormsg));
        return false;
      }
      // Kennung playlist suchen und Name (als Playlist) in Liste einfuegen
      pch = strtok(buf, ":");
      if (!strcmp(pch, "playlist")) {
        pch = strtok(NULL, ":");
        if (pch != NULL) {
          trim(pch);
          mpdPlaylists.insert(ENTRY_TYP_PLAYLIST, pch);
        }
      }
      i=0;
      buf[0] = 0;

    } // if \n

  } // while
  startIdle();
  return true;
}

// ***********************************************
bool MpdClient::get_mpd_db(char* db_level) {
  char c;
  char buf[MAX_RESPONSE_LINE_SIZE];
  char mpd_cmd[1024];
  unsigned int i=0;
  char * pch;
  // alte Liste loeschen
//  mpdDB.clear_list();
  // zuerst MPD-idle ausschalten, sonst wird kein Kommando angenommen
  noIdle();
  // Kommando senden
  sprintf(mpd_cmd, "lsinfo %s", db_level);
  // client.println("lsinfo");
  client.println(mpd_cmd);
  // warten bis etwas kommt
  while(!client.available());
  mpdDB.clear_list();

  // Einlesen und verarbeiten
  while(client.available())  {
    char c = client.read();
    if (c != '\n') {
      if (i < MAX_RESPONSE_LINE_SIZE-1) {
        buf[i] = c;
        buf[i+1] = 0;
        i++;
      }
    } else {
      if(strncmp(buf, "OK", 2)==0){
        // fertig und alles gut
        startIdle();
        //return playlist;
        return true;
      }
      else if (strncmp(buf, "ACK", 3)==0) {
        // ...bis mir besseres einfaellt :-)
        startIdle();
        //return playlist;
        strncpy(mpd_errormsg, buf, sizeof(mpd_errormsg));
        return false;
      }
      pch = strtok(buf, ":");
      if (!strcmp(pch, "directory")) {
        pch = strtok(NULL, ":");
        if (pch != NULL) {
          trim(pch);
          mpdDB.insert(ENTRY_TYP_DIRECTORY, pch);
        }
      }
      if (!strcmp(pch, "file")) {
        pch = strtok(NULL, ":");
        if (pch != NULL) {
          trim(pch);
          mpdDB.insert(ENTRY_TYP_FILE, pch);
        }
      }
      // mpdDB.insert(ENTRY_TYP_DIRECTORY, buf);
      i=0;
      buf[0] = 0;

    } // if \n

  } // while
  startIdle();
  return true;
}

// ***********************************************
MpdPlaylists::MpdPlaylists(){
  head = NULL;
  end = NULL;
  pointer = NULL;
  clear_list();
}

// ***********************************************
MpdPlaylists::~MpdPlaylists() {
  clear_list();
}

// ***********************************************
void MpdPlaylists::insert(uint8_t entry_typ, char *  s){
  playlists_t *tmp = new playlists_t;
  tmp->value = (char*) malloc(strlen(s) + 1);  // ...Fehlerbehandlung schenke ich mir mal...
  strcpy(tmp->value, s);
  tmp->entry_typ = entry_typ;
  tmp->next = NULL;
  if(head == NULL)
  {
    tmp->prev = NULL;
    head = tmp;
    end = tmp;
    pointer = head;
  }
  else
  {
    tmp->prev = end;
    end->next = tmp;
    end = end->next;
  }
}

// ***********************************************
char * MpdPlaylists::get_data(void){
  return pointer->value;
}

// ***********************************************
uint8_t MpdPlaylists::get_data_type(void){
  return pointer->entry_typ;
}

// ***********************************************
bool MpdPlaylists::set_pos_next(void){
  if (pointer->next==NULL) return false;
  pointer = pointer->next;
  return true;
}

// ***********************************************
bool MpdPlaylists::set_pos_prev(void){
  if (pointer->prev==NULL) return false;
  pointer = pointer->prev;
  return true;
}

// ***********************************************
bool MpdPlaylists::set_pos_head(void){
  if (head == NULL) return false;
  pointer = head;
  return true;
}

// ***********************************************
bool MpdPlaylists::set_pos_end(void){
  if (end == NULL) return false;
  pointer = end;
  return true;
}

// ***********************************************
void MpdPlaylists::clear_list(void){
  playlists_t *p = end;
  if (end==NULL) return;
  // von hinten nach vorne loeschen
  while (p->prev != NULL) {
    end = end->prev;
    //free(p->value);
    delete p;
    p = end;
  }
  // letzten auch noch loeschen
  free(p->value);
  delete p;
  head = NULL;
  end  = NULL;
  pointer = NULL;
}

// ***********************************************
bool   MpdPlaylists::is_empty(void){
  if (head == NULL) return true; else return false;
}

// ***********************************************
bool   MpdClient::playlists_set_pos_head(void){
  return mpdPlaylists.set_pos_head();
}

// ***********************************************
bool   MpdClient::playlists_set_pos_end(void){
  return mpdPlaylists.set_pos_end();	
}

// ***********************************************
bool   MpdClient::playlists_set_pos_next(void){
  return mpdPlaylists.set_pos_next();
}

// ***********************************************
bool   MpdClient::playlists_set_pos_prev(void){
  return mpdPlaylists.set_pos_prev();	
}

// ***********************************************
char * MpdClient::playlists_get_data(void){
  return mpdPlaylists.get_data();
}

// ***********************************************
bool   MpdClient::playlists_is_empty(void){
  return mpdPlaylists.is_empty();
}

// ***********************************************
bool   MpdClient::mpd_db_is_empty(void){
  return mpdDB.is_empty();
}

// ***********************************************
bool   MpdClient::mpd_db_set_pos_head(void){
  return mpdDB.set_pos_head();
}

// ***********************************************
bool   MpdClient::mpd_db_set_pos_end(void){
  return mpdDB.set_pos_end();	
}

// ***********************************************
char * MpdClient::mpd_db_get_data(void){
  return mpdDB.get_data();
}

// ***********************************************
uint8_t MpdClient::mpd_db_get_data_type(void){
  return mpdDB.get_data_type();
}

// ***********************************************
bool   MpdClient::mpd_db_set_pos_next(void){
  return mpdDB.set_pos_next();
}

// ***********************************************
bool   MpdClient::mpd_db_set_pos_prev(void){
  return mpdDB.set_pos_prev();
}

