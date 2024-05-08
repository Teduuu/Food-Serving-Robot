

 
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//  MARVELMIND HEDGEHOG RELATED PART

long hedgehog_x, hedgehog_y;// coordinates of hedgehog (X,Y), mm
long hedgehog_z;// height of hedgehog, mm
int hedgehog_pos_updated;// flag of new data from hedgehog received
bool high_resolution_mode;

///

//#define DISTANCE_SENSOR_ENABLED

#define HEDGEHOG_BUF_SIZE 40 
#define HEDGEHOG_CM_DATA_SIZE 0x10
#define HEDGEHOG_MM_DATA_SIZE 0x16
byte hedgehog_serial_buf[HEDGEHOG_BUF_SIZE];
byte hedgehog_serial_buf_ofs;

#define POSITION_DATAGRAM_ID 0x0001
#define POSITION_DATAGRAM_HIGHRES_ID 0x0011
unsigned int hedgehog_data_id;

typedef union {byte b[2]; unsigned int w;int wi;} uni_8x2_16;
typedef union {byte b[4];float f;unsigned long v32;long vi32;} uni_8x4_32;

//    Marvelmind hedgehog support initialize
void setup_hedgehog() 
{
  Serial.begin(115200); // hedgehog transmits data on 115200bps  

  hedgehog_serial_buf_ofs= 0;
  hedgehog_pos_updated= 0;
}

// Marvelmind hedgehog service loop
void loop_hedgehog()
{int incoming_byte;
 int total_received_in_loop;
 int packet_received;
 bool good_byte;
 byte packet_size;
 uni_8x2_16 un16;
 uni_8x4_32 un32;

  total_received_in_loop= 0;
  packet_received= 0;
  
  while(Serial.available() > 0)
    {
      
      if (hedgehog_serial_buf_ofs>=HEDGEHOG_BUF_SIZE) 
      {
        hedgehog_serial_buf_ofs= 0;// restart bufer fill
        break;// buffer overflow
      }
      total_received_in_loop++;
      if (total_received_in_loop>100) break;// too much data without required header
      
      incoming_byte= Serial.read();
     
      good_byte= false;
      switch(hedgehog_serial_buf_ofs)
      {
        case 0:
        {
          good_byte= (incoming_byte = 0xff);
          break;
        }
        case 1:
        {
          good_byte= (incoming_byte = 0x47);
          break;
        }
        case 2:
        {
          good_byte= true;
          break;
        }
        case 3:
        {
          hedgehog_data_id= (((unsigned int) incoming_byte)<<8) + hedgehog_serial_buf[2];
          good_byte=   (hedgehog_data_id == POSITION_DATAGRAM_ID) ||
                       (hedgehog_data_id == POSITION_DATAGRAM_HIGHRES_ID);
          break;
        }
        case 4:
        {
          switch(hedgehog_data_id)
          {
            case POSITION_DATAGRAM_ID:
            {
              good_byte= (incoming_byte == HEDGEHOG_CM_DATA_SIZE);
              break;
            }
            case POSITION_DATAGRAM_HIGHRES_ID:
            {
              good_byte= (incoming_byte == HEDGEHOG_MM_DATA_SIZE);
              break;
            }
          }
          break;
        }
        default:
        {
          good_byte= true;
          break;
        }
      }
      
      if (!good_byte)
        {
          hedgehog_serial_buf_ofs= 0;// restart bufer fill         
          continue;
        }     
      hedgehog_serial_buf[hedgehog_serial_buf_ofs++]= incoming_byte; 
      if (hedgehog_serial_buf_ofs>5)
        {
          packet_size=  7 + hedgehog_serial_buf[4];
          if (hedgehog_serial_buf_ofs == packet_size)
            {// received packet with required header
              packet_received= 1;
              hedgehog_serial_buf_ofs= 0;// restart bufer fill
              break; 
            }
        }
    }

  if (packet_received)  
    {
      hedgehog_set_crc16(&hedgehog_serial_buf[0], packet_size);// calculate CRC checksum of packet
      if ((hedgehog_serial_buf[packet_size] == 0)&&(hedgehog_serial_buf[packet_size+1] == 0))
        {// checksum success
          switch(hedgehog_data_id)
          {
            case POSITION_DATAGRAM_ID:
            {
              // coordinates of hedgehog (X,Y), cm ==> mm
              un16.b[0]= hedgehog_serial_buf[9];
              un16.b[1]= hedgehog_serial_buf[10];
              hedgehog_x= 10*long(un16.wi);
              un16.b[0]= hedgehog_serial_buf[11];
              un16.b[1]= hedgehog_serial_buf[12];
              hedgehog_y= 10*long(un16.wi);              
              // height of hedgehog, cm==>mm (FW V3.97+)
              un16.b[0]= hedgehog_serial_buf[13];
              un16.b[1]= hedgehog_serial_buf[14];
              hedgehog_z= 10*long(un16.wi);
              
              hedgehog_pos_updated= 1;// flag of new data from hedgehog received
              high_resolution_mode= false;
              break;
            }

            case POSITION_DATAGRAM_HIGHRES_ID:
            {
              // coordinates of hedgehog (X,Y), mm
              un32.b[0]= hedgehog_serial_buf[9];
              un32.b[1]= hedgehog_serial_buf[10];
              un32.b[2]= hedgehog_serial_buf[11];
              un32.b[3]= hedgehog_serial_buf[12];
              hedgehog_x= un32.vi32;

              un32.b[0]= hedgehog_serial_buf[13];
              un32.b[1]= hedgehog_serial_buf[14];
              un32.b[2]= hedgehog_serial_buf[15];
              un32.b[3]= hedgehog_serial_buf[16];
              hedgehog_y= un32.vi32;
              
              // height of hedgehog, mm 
              un32.b[0]= hedgehog_serial_buf[17];
              un32.b[1]= hedgehog_serial_buf[18];
              un32.b[2]= hedgehog_serial_buf[19];
              un32.b[3]= hedgehog_serial_buf[20];
              hedgehog_z= un32.vi32;
              
              hedgehog_pos_updated= 1;// flag of new data from hedgehog received
              high_resolution_mode= true;
              break;
            }
          }
        } 
    }
}

// Calculate CRC-16 of hedgehog packet
void hedgehog_set_crc16(byte *buf, byte size)
{uni_8x2_16 sum;
 byte shift_cnt;
 byte byte_cnt;

  sum.w=0xffffU;

  for(byte_cnt=size; byte_cnt>0; byte_cnt--)
   {
   sum.w=(unsigned int) ((sum.w/256U)*256U + ((sum.w%256U)^(buf[size-byte_cnt])));

     for(shift_cnt=0; shift_cnt<8; shift_cnt++)
       {
         if((sum.w&0x1)==1) sum.w=(unsigned int)((sum.w>>1)^0xa001U);
                       else sum.w>>=1;
       }
   }

  buf[size]=sum.b[0];
  buf[size+1]=sum.b[1];// little endian
}// hedgehog_set_crc16

//  END OF MARVELMIND HEDGEHOG RELATED PART
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

#include <Wire.h>
#define CM 1      //Centimeter
#define INC 0     //Inch
#define TP 2      //Trig_pin
#define EP 3      //Echo_pin
#define row 5
#define col 11

//============================================宣告初始變數================================================================//
int wait = 4;
int desk_x = 2;
int desk_y = 9;
int save_x = 2;       //save x from the past
int save_y = 9;
float theta_current;
int break_range = 3; //distanse that the car will break near target
int arrive = 0;
int arrive_next = 0;
char car_status = 's';// s = standby門口待命 ; r = running去餐桌 ; t = takingfood去廚房 ; f = foodserving餐桌服務 ; k = kitchen廚房等餐 ; b = back回餐桌; d = diliver送餐
int go_to_seat = 0;
int go_to_seat_pass1 = 0;
long hedgehog_x_pass = 0;
long hedgehog_y_pass = 0;
//===============================================A* 變數======================================================//
int goalN; // goal position on grid 目標地點
int starting; //starting position on grid 起點
int curBotPos; // holds current bot position
int openList[60]; // contains all the possible paths
int closedList[60]; // contains the path taken
int neighborList[10]; //放置周圍的鄰居點
int check1_neighborList[10]; //放置檢查後周圍的鄰居點(可以執行的)
int check2_neighborList[10]; //放置檢查後周圍的鄰居點(可以執行的) //最後檢查後都是用這個list(用鄰居點時)
int check2_neighborList_index;
int oLN = 0, cLN = 0, bM = 0; // the counters for the openList, closedList and the bot's movement
int obstacle[] = {0,0,0,0,23};
int obstacle_1[3] = {};
int choose_f_least_point; //存最小的f
int run = 0 ; // 用在choose_f_least_point_function
int A_star_list_index = 0;
int *A_star_list_index_pointer;
int A_star_x ; //下一個點
int A_star_y ; //下一個點
int A_star_x_real;
int A_star_y_real;
int A_star_test = 0;
struct Node
{
  int g, h, f; //only save 0~255
  int parent;
  int index;
  int gridNom;
};

struct Grid
{
  Node Map[row][col];
} PF ;
//========================================A*的變數到這邊==================================================//

int Table_x[6] = {2,1,1,3,3,2};
int Table_y[6] = {9,7,3,7,3,1};


int axis_x,axis_y;//現在的位置
long edge_x = 4580;
long edge_y = -3900;
//=====================================================宣告初始變數=======================================================//



void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  setup_hedgehog();//    Marvelmind hedgehog support initialize
  Wire.begin(8);                // join i2c bus with address #8
  Wire.onRequest(requestEvent); // register event
  Wire.onReceive(receiveEvent); // register 
  axis_x=2;  //起點
  axis_y=9;
}

void loop() 
{
  // put your main code here, to run repeatedly:
//=========================================計算方向==================================================================//

    /*Serial.print("A_star_x:");
    Serial.println(A_star_x);
    Serial.print("A_star_y:");
    Serial.println(A_star_y);*/
   

    
    loop_hedgehog();// Marvelmind hedgehog service loop

    if(hedgehog_x == 0)
    {
      hedgehog_x = hedgehog_x_pass;
    }
    else
    {
      hedgehog_x_pass = hedgehog_x;
    }
    if(hedgehog_y == 0)
    {
      hedgehog_y = hedgehog_y_pass;
    }
    else
    {
      hedgehog_y_pass = hedgehog_y;
    }
    
    axis_y = (edge_x-hedgehog_x)/800;  // 現在位置轉成A*地圖上的座標
    axis_x = (hedgehog_y-edge_y)/800;
    if(axis_x<0)
    {
      axis_x = 0;
    }
    if(axis_x> 4)
    {
      axis_x = 4;
    }
    if(axis_y<0)
    {
      axis_y = 0;
    }
    if(axis_y> 10)
    {
      axis_y = 10;
    }    


//=============================================改座標時================================================================//
 
    
    if(go_to_seat!=go_to_seat_pass1)//如果目標桌號發生改變(準備開始動)
    {
      desk_x = Table_x[go_to_seat]; //要去的桌號座標
      desk_y = Table_y[go_to_seat]; 
      //A_star();
      wait = 1;
      if(car_status=='s') //在門口待命時
      { 
        if(go_to_seat == 5)
        {
          car_status = 't'; //去廚房
        }
        else
        {
          car_status = 'r'; //去餐桌
        }
      }
      else if(car_status=='k') //在廚房等餐時
      {
        car_status = 'd'; //去餐桌
      }
      else if(car_status=='f') //在餐桌服務時
      {
        car_status = 'b'; //回門口待命
      }
       
   }   

      go_to_seat_pass1 = go_to_seat;
   
//===============================改座標時========================================================================//


  Serial.println("======================");
    Serial.print("desk_x:");//去那桌的座標
    Serial.println(desk_x);
    Serial.print("desk_y:");
    Serial.println(desk_y);
    Serial.print("axis_x:");
    Serial.println(axis_x);
    Serial.print("axis_y:");
    Serial.println(axis_y);


    
    arrive = check_if_arrive(axis_x,axis_y,desk_x,desk_y);
    
    if (arrive==1)  //若到目的地
    { 
      Serial.println("arrived"); 
      if(car_status == 't')  //若到廚房
      { 
        car_status = 'k';  
      }
      else if (car_status == 'k') //等待送餐位置
      { 
        car_status = 'k';
      }
      else if (car_status == 'r') //若到餐桌
      { 
        car_status = 'f';
      }
      else if (car_status == 'f') //等待點餐完畢
      { 
        car_status = 'f';
      }
      else if  (car_status == 's') //若到門口
      { 
        car_status = 's';
      }
      else if  (car_status == 'b') //若到門口
      { 
        car_status = 's';
      }
      else if  (car_status == 'd') //若到餐桌
      { 
        car_status = 'f';
      }
      theta_current = 0;
      
    }
    else //計算角度給馬達
    { 
      /*A_star_x = A_star_list_index_pointer[A_star_list_index]/col; //下一步座標
		  A_star_y = A_star_list_index_pointer[A_star_list_index]%col; 
		  arrive_next = check_if_arrive(axis_x,axis_y,A_star_x,A_star_y);
		  if(arrive_next==1 && A_star_list_index>0)
		  {
		    A_star_list_index -=1;
		  }
     */
     if(wait>3)
     {
      A_star_x_real = 4580-800*desk_y -400;
      A_star_y_real = -3900+800*desk_x + 400;
  Serial.print("go to seat:");
    Serial.println(go_to_seat);
  Serial.print("car_status：");
    Serial.println(car_status);
    Serial.print("hedgehog_x:");
    Serial.println(hedgehog_x);
  Serial.print("hedgehog_y：");
    Serial.println(hedgehog_y);
  Serial.print("A_star_x_real：");
    Serial.println(A_star_x_real);
  Serial.print("A_star_y_real：");             
    Serial.println(A_star_y_real);
    Serial.print("save_x：");              
    Serial.println(save_x);
    Serial.print("save_y：");              
    Serial.println(save_y);
		  theta_current = calculate_angle(save_x,save_y,hedgehog_x,hedgehog_y)- calculate_angle(hedgehog_x,hedgehog_y,A_star_x_real,A_star_y_real);
      if(theta_current>180)
      {
        theta_current = theta_current-360; 
      }
      else if(theta_current<-180)
      {
        theta_current = theta_current+360;
      }
      if(theta_current > 90)
      {
        theta_current = 90;
      }
      else if(theta_current < -90)
      {
        theta_current = -90;
      }
      else
      {
        theta_current = theta_current;
      }
     }
     else
     {
      theta_current = 0;
     }
      Serial.print("theta_current：");
      Serial.println(theta_current);
        
		
    }
    wait+=1;
    if(wait>10)
    {
      wait = 4;
    }
       
//=============================================計算方向==================================================================//



//=============================存上個xy=======================================//
  save_x=hedgehog_x;
  save_y=hedgehog_y;
//============================存上個xy=====================================//
  delay(200);
  
}//end loop


//===================================================傳輸===================================================================//
void requestEvent() //傳status過去esp32
{ 
  int theta_send = theta_current;
  Wire.write(car_status);
  Wire.write(theta_send); // respond with message of 4 ints
}
int seat = 0,seat_pass = 0;
void receiveEvent(int howMany)
{    
  seat = Wire.read();
  
  if(seat_pass == seat)
  {
    go_to_seat = seat;
  }
  seat_pass = seat;        
}
//==========================================================傳輸================================================================//


//=============================================================GPS===============================================================//
long Distance(long time, int flag)
{

  long distacne;
  if(flag)
    distacne = time /29 / 2  ;     // Distance_CM  = ((Duration of high level)*(Sonic :340m/s))/2
                                   //              = ((Duration of high level)*(Sonic :0.034 cm/us))/2
                                   //              = ((Duration of high level)/(Sonic :29.4 cm/us))/2
  else
    distacne = time / 74 / 2;      // INC
  return distacne;
}

long TP_init()
{                     
  digitalWrite(TP, LOW);                    
  delayMicroseconds(2);
  digitalWrite(TP, HIGH);                 // pull the Trig pin to high level for more than 10us impulse 
  delayMicroseconds(10);
  digitalWrite(TP, LOW);
  long microseconds = pulseIn(EP,HIGH);   // waits for the pin to go HIGH, and returns the length of the pulse in microseconds
  return microseconds;                    // return microseconds
}
//=======================================================GPS===============================================================//


//============================計算到目標的角度====================================//
float calculate_angle(float a,float b,float c,float d)
{
  float theta_tar;
  float dx=c-a;//delta x
  float dy=d-b;
  float abs_dx = abs(dx)+1;
  float abs_dy = abs(dy);
  float tangent_value=abs_dy/abs_dx;
  theta_tar=atan(tangent_value)*180/PI;  // arc tangent of x 弧度
  if(dx>0 && dy<0){
    theta_tar = 360- theta_tar ;
    }
  else if(dx<0 && dy<0){
    theta_tar =180 + theta_tar;
    }
  else if(dx<0 && dy>0){
    theta_tar =180 - theta_tar;
    }
  else if(dx<0 && dy>0){
    theta_tar = theta_tar;
    }
  return theta_tar;
}
//========================計算到目標的角度============================================//

//===============================================距離控制======================================================//

int check_if_arrive(int a,int b,int c,int d)
{ 
  int x_abs = c-a;
  int y_abs = d-b;
  x_abs = abs(x_abs);
  y_abs = abs(y_abs); //距離絕對值

  if( x_abs < break_range && y_abs < break_range ) //when distance < range
  {
    return 1;
  }
  else
  {
    return 0;
  }
}
//========================================================距離控制================================================//

//==========================================================A*================================================//
void buildMap() // builds the 10x10 map grid 建立地圖
{
  for (int i = 0; i < row; i++)  //row 10
  {
    for (int j = 0; j < col; j++)  ////col 21
    {
      PF.Map[i][j].gridNom = i*col+j; //將格子編號
      PF.Map[i][j].index = 0;
      PF.Map[i][j].parent = 0;
      PF.Map[i][j].h = 0;
      PF.Map[i][j].g = 0;
      PF.Map[i][j].f = 0;
     }
   }
}

void A_star(){ // performs the A* algorithm, "main" program
  Serial.println("A_star is begin");
  A_star_test+=1;
  initialize();
  A_star_test+=1;
  buildMap();  //建立地圖
  A_star_test+=1;
  setGoal();
  A_star_test+=1;
  AddOpenList(starting);//把起點加入openlist 
  A_star_test+=1;
  while(curBotPos != goalN)//curBotPos != goalN
  {
    choose_f_least_point = choose_f_least_point_function(); //找出 f  least ， choose_f_least_point就是現在的點(current)
    remove_f_least_point_from_oppenlist(choose_f_least_point);
    curBotPos = choose_f_least_point;//讓curBotPos同時也等於choose_f_least_point
    if(curBotPos == goalN)
    {
      break;
    }
    find_neighbor_point();//找出鄰居點
    AddClosedList(choose_f_least_point);//把current(choose_f_least_point)加入closed list
    check_neighbor_point();//鄰居點每個都做，如果這個鄰居點不能走，or在closed list 中則刪除
    for (int i = 0 ; i< check2_neighborList_index ; i++) //每個鄰居點都做
    {
      if( in_openlist_or_not(check2_neighborList[i]) )//如果鄰居點在openlist中，則做......(我的筆記的第四步) //檢查是否有在openlist裡面
      {
        //做鄰居點"在"openlist中的事情
        if( check_g_value_with_current_and_neighbor(check2_neighborList[i],choose_f_least_point) )  //這裡面的函式用來確認"鄰居點的g值比current還要小" openList[i].g(鄰居點) < choose_f_least_point(current)
        {
          //如果鄰居點的g值加了temp比current還要小
          set_neighbor_current(check2_neighborList[i],choose_f_least_point); //將current設為鄰居點的parrent
          heuristics(check2_neighborList[i]); 
        }
        else
        {
          //如果鄰居點的g值"沒有"比current還要小 (neighbor比較大)
          //好像什麼都不用做(不是很確定cc ><)
        }
      } 
      else
      {
        //做鄰居點"不在"openlist中的事情 
        set_neighbor_current(check2_neighborList[i],choose_f_least_point);//把現在的點(current)設成鄰居點的父節點 。輸入：鄰居點(check2_neighborList[i])，現在的點(choose_f_least_point)
        AddOpenList(check2_neighborList[i]);//把此點加入openlist,輸入：要輸入點的座標  註：已經把鄰居點的f,g,h算完了
      }
    }
    clear_neighbor_list(); //每做完一次，要清空neighbor_list(清空三個list)neighborList check1_neighborList check2_neighborList
  }
  A_star_test+=1;
  //以下是開始生成陣列出來
  //A_star_list[A_star_list_index] = goalN;
  A_star_list_index = A_star_list_index +1 ;
  int goalN_units_digit,goalN_tens_digit; //goalN的個位數,十位數
  goalN_tens_digit = goalN / col; //goalN的十位數
  goalN_units_digit = goalN % col; //goalN的個位數
  int temp;
  temp = PF.Map[goalN_tens_digit][goalN_units_digit].parent;
  int temp_units_digit,temp_tens_digit; //temp的個位數,十位數
  while(temp != starting)
  {
    //A_star_list[A_star_list_index] = temp;
    A_star_list_index = A_star_list_index +1 ;
    temp_tens_digit = temp / col; //temp的十位數
    temp_units_digit = temp % col; //temp的個位數
    temp = PF.Map[temp_tens_digit][temp_units_digit].parent;
  }
  A_star_list_index_pointer= new int [A_star_list_index];
  //Serial.println("A_star_list_index");
  //Serial.println(A_star_list_index);
  //存進去
  A_star_list_index_pointer[0] = goalN;
  goalN_tens_digit = goalN / col; //goalN的十位數
  goalN_units_digit = goalN % col; //goalN的個位數
  temp = PF.Map[goalN_tens_digit][goalN_units_digit].parent;
  for(int i=1;i<A_star_list_index;i++) //A_star_list_index=5
  {
    A_star_list_index_pointer[i] = temp;
    temp_tens_digit = temp / col; //temp的十位數
    temp_units_digit = temp % col; //temp的個位數
    temp = PF.Map[temp_tens_digit][temp_units_digit].parent;
  }
  A_star_list_index_pointer[A_star_list_index]=starting;
  Serial.println("A_star_list_index_pointer");
  for(int i=0;i<=A_star_list_index;i++)
  {
    Serial.println(A_star_list_index_pointer[i]);
  }
  Serial.println("A_star is over");
}

void initialize()
{
  starting = axis_x *col + axis_y; //起點
  goalN = desk_x*col + desk_y;
  for(int i = 0;i < 60 ;i++)  //initialize openList and closedList
  {
    openList[i] = 0; 
    closedList[i] = 0;
  }
  clear_neighbor_list(); //要清空neighbor_list(清空三個list)neighborList check1_neighborList check2_neighborList
  oLN = 0, cLN = 0, bM = 0;
  run = 0 ; // 用在choose_f_least_point_function
  //for(int i = 0;i < A_star_list_index ;i++)  //initialize openList and closedList
  //{
    //A_star_list[i] = 0;
  //}
  A_star_list_index = 0;
  delete A_star_list_index_pointer;
  A_star_list_index_pointer=NULL;
  
  int temp=0;
  for(int i = 0;i <= 3 ;i++)
	{
		obstacle_1[i] = 0 ;
	}
  
	for(int i = 0;i <= 4 ;i++)
	{
		if(goalN != obstacle[i])
		{
			obstacle_1[temp] = obstacle[i];
			temp = temp + 1;
		}
	}
}

void setGoal() // asks user for input to set the goal state/tile 請使用者打要去地方(目標),並且設定每個點的狀態(1,2,3)
{
  
  for (int i = 0; i < row; i++)
  {
    for (int k = 0; k < col; k++)
    {
      if (PF.Map[i][k].gridNom == goalN)  //接下來是設定每個點的inde,1是起點,2是障礙,3是終點
      {
        PF.Map[i][k].index = 3;  //設成目標
        goalN = PF.Map[i][k].gridNom;  // goalN是goal position on grid
      }
      else if (PF.Map[i][k].gridNom == starting) //45為起始點
      {
        PF.Map[i][k].index = 1;
        curBotPos = PF.Map[i][k].gridNom;
      }
      else if (PF.Map[i][k].gridNom == 0 || PF.Map[i][k].gridNom == 10 ||PF.Map[i][k].gridNom == 44  || PF.Map[i][k].gridNom == 54   )//這些為障礙物
      {
        PF.Map[i][k].index = 2;
      }
      else
        PF.Map[i][k].index = 0;
    }
  }
}

int choose_f_least_point_function() //return f least
{
  int temp;    
  if( run == 0 )
  {
    temp = openList[0];
    run = run + 1;
  }
  else
  {
    temp = openList[0];
    for(int i = 1;i < oLN ;i++)
    {
      if( (get_openList_f_value(temp) == get_openList_f_value(openList[i]))  ) //如果f值一樣|| (get_openList_f_value(temp) < get_openList_f_value(openList[i]))
      {
        if(get_openList_h_value(openList[i]) < get_openList_h_value(temp)) //就要繼續用h值判斷
        {
          temp = openList[i] ;    //要等於點的座標
        }

      }
      else if( get_openList_f_value(openList[i]) < get_openList_f_value(temp) )//如果出現更小的(用f來比較大小)  
      {
        temp = openList[i] ;    //要等於點的座標
      }
    }
  }
  return temp ;
}

int get_openList_f_value(int openList_point) //回傳點的f值 輸入：openlist的點 回傳：點的f值
{
  int openList_point_units_digit,openList_point_tens_digit;//openList的個位數,十位數
  openList_point_tens_digit = openList_point / col;//openList的十位數
  openList_point_units_digit = openList_point % col;//openList的個位數
  int openList_f_value;
  openList_f_value = PF.Map[openList_point_tens_digit][openList_point_units_digit].f;
  return openList_f_value;
}

int get_openList_h_value(int openList_point) //回傳點的h值 輸入：openlist的點 回傳：點的h值
{
  int openList_point_units_digit,openList_point_tens_digit;//openList的個位數,十位數
  openList_point_tens_digit = openList_point / col;//openList的十位數
  openList_point_units_digit = openList_point % col;//openList的個位數
  int openList_h_value;
  openList_h_value = PF.Map[openList_point_tens_digit][openList_point_units_digit].h;
  return openList_h_value;
}

void remove_f_least_point_from_oppenlist(int choose_f_least_point)
{
  int i = 0;
  for(i = 0 ; i < oLN ; i++)
  {
    if( openList[i] != choose_f_least_point )
    {
      openList[i] = openList[i];
    }
    else
    {
      openList[i] = openList[i+1];
      i = i + 1;
      oLN = oLN -1;
      break;
    }
  }
  for(int j = i ; j < oLN ; j++)
  {
    openList[j] = openList[j+1];
  }
}

void find_neighbor_point()//找出鄰居點
{
  int temp = 0;
  for(int i = curBotPos-12;i <= curBotPos-10 ;i++)
  {
    neighborList[temp] = i;
    temp ++;
  }
  neighborList[temp] = curBotPos-1;
  temp ++;
  neighborList[temp] = curBotPos+1;
  temp ++;
  for(int i = curBotPos+10;i <= curBotPos+12 ;i++)
  {
    neighborList[temp] = i;
    temp ++;
  }
}

void check_neighbor_point()//鄰居點每個都做，如果這個鄰居點不能走，or在closed list 中則刪除
{
  int counter = 0;
  for(int i = 0 ; i < 8; i++) //遇到障礙刪除
  {
    if(check_obstacle( neighborList[i] ))
    {
      check1_neighborList[counter] = neighborList[i];//如果沒在裡面就複製過去
      counter++;
    }
  }
  check2_neighborList_index = 0;
  for(int i = 0 ; i < counter ; i++)
  {
    if( check_closelist(check1_neighborList[i]) )
    {
      check2_neighborList[check2_neighborList_index] = check1_neighborList[i];
      check2_neighborList_index++;
    }
  }
}

bool check_obstacle( int check_number )//檢查有沒有在obstacle,輸入是欲檢查的點,回傳true或是false
{
  for(int i = 0 ; i < sizeof(obstacle_1) / sizeof(int) ; i++)
  {
    if(check_number == obstacle_1[i])
    {
      return false;
    }
  }
  return true;
}

void clear_neighbor_list()//每做完一次，要清空neighbor_list(清空三個list)
{
  for(int i = 0 ; i < 10 ; i++ )
  {
    neighborList[i] == 0 ;
    check1_neighborList[i] == 0 ;
    check2_neighborList[i] == 0 ;
  }
}


bool check_closelist(int check_number)//檢查有沒有在closelist,輸入是欲檢查的點,回傳true或是false
{
  for(int i = 0 ; i<cLN ; i++)
  {
    if(check_number == closedList[i])
    {
      return false;
    }
  }
  return true;
}

bool in_openlist_or_not(int number)//檢查是否有在openlist裡面(一個看)
{
  for(int i = 0 ; i < oLN ; i++)
  {
    if(number == openList[i])
    {
      return true;
    }
  }
  return false;
}

bool check_g_value_with_current_and_neighbor(int neighbor, int current)//確認"鄰居點的g值比current還要小"   
{
  //輸入:鄰居點,current  回傳:true(current的g值加了temp比鄰居點還要小) false(current的g值加了temp沒有比鄰居點還要小)
  //要先嘗試計算新的g，再比較
  int neighbor_units_digit,neighbor_tens_digit;//鄰居點的個位數,十位數
  neighbor_tens_digit = neighbor / col;//鄰居點的十位數
  neighbor_units_digit = neighbor % col;//鄰居點的個位數
  int current_units_digit,current_tens_digit;//鄰居點的個位數,十位數
  current_tens_digit = current / col;//鄰居點的十位數
  current_units_digit = current % col;//鄰居點的個位數
  
  if( PF.Map[neighbor_tens_digit][neighbor_units_digit].g > (PF.Map[current_tens_digit][current_units_digit].g + temp(neighbor_tens_digit,neighbor_units_digit,current_tens_digit,current_units_digit) ) )
  {
    return true;
  }
  else 
  {
    return false;
  }
}

int temp(int neighbor_tens_digit,int neighbor_units_digit,int current_tens_digit,int current_units_digit)
{
  if(abs(neighbor_tens_digit - current_tens_digit) == abs(neighbor_units_digit - current_units_digit))
  {
    return 14;
  }
  else
  {
    return 10;
  }
}

void set_neighbor_current(int i,int current)//把現在的點(current)設成鄰居點的父節點，輸入：鄰居點(i)，現在的點(choose_f_least_point)
{
  int units_digit,tens_digit;//個位數,十位數
  tens_digit = i / col;//十位數
  units_digit = i % col;//個位數
  PF.Map[tens_digit][units_digit].parent = current;  //現在的點(current)設成鄰居點的父節點
}


void AddOpenList(int aol) // adds the potential possible moves to the openList 輸入：要輸入點的座標
{ 
  if(aol == starting)
  {
    openList[oLN] = aol;   //將此點加入openList
    oLN = oLN +1;
  }
  else
  {
    openList[oLN] = aol;   //將此點加入openList
    oLN = oLN +1;
    heuristics(aol); //heuristics函式，傳點進去
  }
}

void AddClosedList(int curIn)//將此點加入closedlist
{
  closedList[cLN] = curIn;//將此點加入closedlist
  cLN = cLN + 1;
}

void heuristics(int curIn) // calculates the "cost" of the tile
{
  int hH, gH, fH;
  int rowh = (int) curIn / col; //十位數
  int colh = curIn % col;  //個位數

  hH = H(rowh, colh, goalN);
  PF.Map[rowh][colh].h = hH;
  gH = G(rowh, colh);
  PF.Map[rowh][colh].g = gH;
  fH = FV(hH,gH);
  PF.Map[rowh][colh].f = fH;
}


int H(int curR, int curC, int goalS)  // manhattan distance heauristics function 輸入：傳當前的x,y位置,終點的位置
{
  int rowg, colg;
  int manhattan=0;
  rowg = (int)goalS/col; //終點的座標(類似x座標)
  colg = goalS%col;       //終點的座標(類似y座標)  
  int x_direct_diffenence = abs(curR - rowg);
  int y_direct_diffenence = abs(curC - colg);
  if(x_direct_diffenence>y_direct_diffenence)
  {
    manhattan += (10*(x_direct_diffenence-y_direct_diffenence)+14*y_direct_diffenence);
  }
  else
  {
    manhattan += (10*(y_direct_diffenence-x_direct_diffenence)+14*x_direct_diffenence);
  }
  return manhattan;
}

int G(int curR, int curC)  // returns the "depth" level of the tile //傳入要計算g值的點的十位數和個位數
{
  int gValue, parInd; //宣告變數
  int rowg, colg;   //宣告變數
  
  parInd = PF.Map[curR][curC].parent; //找出父節點的座標
  
  rowg = parInd/col; //parent的十位數
  colg = parInd%col;//parent的個位數
  gValue = PF.Map[rowg][colg].g;//當前位置的g值
  
  if( abs(curR-rowg) == abs(curC-colg) )
  {
    gValue += 14;
  }
  else
  {
    gValue +=10;
  }
  return gValue;
}

int FV(int curG, int curH) //加起來 the "cost" of the path taken; adds H and G values for each tile 
{
  int fValue; 
  fValue = curG + curH;
  return fValue;
}
//===============================================A*==========================================================//
