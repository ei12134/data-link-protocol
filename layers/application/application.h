#ifndef APPLICATION_H_
#define APPLICATION_H_

#define MAX_DATA_BYTES 256

struct spFrame {
  char flags[3];
};

struct infoFrame {
  char prefix[3];
  char *data;
  char suffix[1];
};

struct dataPacket {
  char control_field;
  char seq_num;
  char l1; /* number of bytes read % MAX_DATA_BYTES */
  char l2; /* number of bytes read / MAX_DATA_BYTES */
  char *data_packet;
};

struct controlPacket {
  char control_field;

  /* TLV (Type, Length, Value) */
  char t1;  /* 0 - type = file size */
  char l1;  /* v1 length */
  char *v1; /* file size value */
  char t2;  /* 1 - type = filename */
  char l2;  /* v2 length */
  char *v2; /* filename value */
};

struct spFrame createSupervisionFrame(char address, char control,
                                      char protection);

struct infoFrame createInformationFrame(char address, char control,
                                        char protection1, char *data,
                                        char protection2);

struct dataPacket createDataPacket(char control_field, char seq_num, char l1,
                                   char l2, char *data_packet);

struct controlPacket createControlPacket(char control_field, char t1, char l1,
                                         char *v1, char t2, char l2, char *v2);

#endif // APPLICATION_H_
