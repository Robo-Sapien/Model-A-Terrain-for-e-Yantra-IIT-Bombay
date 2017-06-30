#include <stdio.h>
#include <math.h>

//Function definition
void analyze_object();
void check_node();
void initialize_motion();
void prepare_motion(foreward_flag,left_flag,right_flag);
void update_path_array(foreward_flag,left_flag,right_flag)
void update_object_array(int object_flag,char color,char side);
int direction2num(char direction);

//Structs used
typedef struct{
	int object_flag;	//Shows if the object is present or notin that cell
	char r_color;		//Give the color of that face(r,g,b,bl:black)
	char l_color;		//done in static cordinate as in the picture
	char u_color;
	char d_color;
}object;
typedef struct{
	int r_free;			//done in static cordinate(as in picture)
	int l_free;
	int u_free;
	int d_free;
}node_path;
typedef struct{
	int node_x;			//last node visited ka x_cordinate
	int node_y;			//last node visited ka y_cordinate
	char direction;		//(u,d,l,r) according to our fixed cordinate
	char present;		//(m,n) where m:mid,n:node
}pos_vector;
typedef struct{
	int r;
	int g;
	int b;
}threshold;

void initialize_position(pos_vector *bot_pos);
//global Variables
object object_array[5][5];
node_path path_array[6][6];
int dist_counter_1=0;
int dist_counter_2=0;
pos_vector bot_pos;
int main(void)
{
	initialize_position(&bot_pos);								//DONE
    bot_pos.node_x=2;
    bot_pos.node_y=2;
    
    printf("%c\n",bot_pos.direction);
    update_object_array(1,'r','r');
    printf("Entries:%d,%c\n",object_array[2][2].object_flag,object_array[2][2].l_color);
	while (1)
	{
		initialize_motion();
		
		if (dist_counter_1==20)
		{
			dist_counter_1=0;
			analyze_object();
		}
		initialize_motion();									//just start travelling path
		if (dist_counter_2=20)
		{
			dist_counter_2=0;
			check_node();
		}
		prepare_motion();										//orient in necessary direction
	}
	return 0;
}

void analyze_object()
{
	char color;
	int left_ir=0,right_ir=0;

	left_ir=check_left_ir();									//BHAGAT
	right_ir=check_right_ir();									//BHAGAT
	if (left_ir)
	{
		rotate_servo_left();									//BHAGAT
		color=check_face_color();								//BHAGAT
        update_object_array(1,color,'l');
	}
    else{
        update_object_array(0,color,'l');
    }
	if (right_ir)
	{
		rotate_servo_right();									//BHAGAT
		color=check_face_color();								//BHAGAT
        update_object_array(1,color'r');
	}
    else{
        update_object_array(0,color'r');
    }
}

void check_node()
{
    foreward_flag=checkforeward_IR();                          //Bot's reference BHAGAT
    left_flag=checkleft_IR();
    right_flag=checkleft_IR();
    update_path_array(foreward_flag,left_flag,right_flag);
}

void update_path_array(foreward_flag,left_flag,right_flag){
    dir=bot_pos.direction;
    x=bot_pos.node_x;
    y=bot_pos.node_y;
    if (dir=='l'){
        path_array[x][y].l_free=foreward_flag;
        path_array[x][y].r_free=1;
        path_array[x][y].d_free=left_flag;
        path_array[x][y].u_free=right_flag;
    }
    else if(dir=='r'){
        path_array[x][y].r_free=foreward_flag;
        path_array[x][y].l_free=1;
        path_array[x][y].u_free=left_flag;
        path_array[x][y].d_free=right_flag;
    }
    else if(dir=='u'){
        path_array[x][y].u_free=foreward_flag;
        path_array[x][y].l_free=left_flag;
        path_array[x][y].r_free=right_flag;
        path_array[x][y].d_flag=1;
    }
    else if(dir=='d'){
        path_array[x][y].d_free=foreward_flag;
        path_array[x][y].f_free=1;
        path_array[x][y].r_free=left_flag;
        path_array[x][y].l_free=right_flag;
    }
}

int direction2num(char direction){
    if (direction=='l' || direction=='d'){
        return -1;
    }
    else{
        return 1;
    }
}

void update_object_array(int object_flag,char color,char side)
{
    int x,y;
    int x_add=0,y_add=0;
    int face_lift_x=0,face_lift_y=0;
    if (bot_pos.direction=='l'){
        x_add=-1;
        if (side=='l'){
            face_lift_y=-1;
            object_array[bot_pos.node_x+x_add+face_lift_x][bot_pos.node_y+y_add+face_lift_y].u_color=color;
        }
        else{
            printf("%d,%d\n",bot_pos.node_x+x_add+face_lift_x,bot_pos.node_y+y_add+face_lift_y);
            object_array[bot_pos.node_x+x_add+face_lift_x][bot_pos.node_y+y_add+face_lift_y].d_color=color;
            
        }
    }
    else if(bot_pos.direction=='d'){
        y_add=-1;
        if (side=='r'){
            face_lift_x=-1;
            object_array[bot_pos.node_x+x_add+face_lift_x][bot_pos.node_y+y_add+face_lift_y].r_color=color;
        }
        else{
            object_array[bot_pos.node_x+x_add+face_lift_x][bot_pos.node_y+y_add+face_lift_y].l_color=color;
        }
    }
    else if(bot_pos.direction=='u'){
        if (side=='l'){
            face_lift_x=-1; 
            object_array[bot_pos.node_x+x_add+face_lift_x][bot_pos.node_y+y_add+face_lift_y].r_color=color;
        }
        else{
            object_array[bot_pos.node_x+x_add+face_lift_x][bot_pos.node_y+y_add+face_lift_y].l_color=color;
        }
    }
    else{
        if (side=='r'){
            //printf("Hi");
            face_lift_y=-1;
            object_array[bot_pos.node_x+x_add+face_lift_x][bot_pos.node_y+y_add+face_lift_y].u_color=color;
        }
        else{
            object_array[bot_pos.node_x+x_add+face_lift_x][bot_pos.node_y+y_add+face_lift_y].d_color=color;
        }
    }
    
    x=bot_pos.node_x+x_add+face_lift_x;
    y=bot_pos.node_y+y_add+face_lift_y;
    
    //Updating
    if (object_flag==1){
        object_array[x][y].object_flag=1; 
    }
    else{
        object_array[x][y].object_flag=0;
    }
}

void initialize_position(pos_vector *bot_pos)
{
    //printf("HI");
	bot_pos->node_x=0;
	bot_pos->node_y=0;
	bot_pos->direction='l';
	bot_pos->present='n';
}

void prepare_motion(foreward_flag,left_flag,right_flag)
{
    //We will be following a row by row pattern or column by column pattern. So in case a obstacle is detected we have to crossover that obstacle(also
    //  it is a boon as we will simultaneously detect all the obstacle around the shape and model them all.)
    if (foreward_flag==1 && bot_pos.direction=='l'){
        
    }
}
