/* This is a solo paranoia game taken from the Jan/Feb issue (No 77) of
   "SpaceGamer/FantasyGamer" magazine.

   Article by Sam Shirley.

   Implemented in C on Vax 11/780 under UNIX by Tim Lister

   This is a public domain adventure and may not be sold for profit */

#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include "paranoia.h"
#include "renderer.h"
#include "SDL.h"

#define MOXIE	13
#define AGILITY	15
#define MAXKILL  7	/* The maximum number of UV's you can kill */

int intro = 1;
int clone = 1;
int page = 1;
int computer_request = 0;
int ultra_violet = 0;
int action_doll = 0;
int hit_points = 10;
int read_letter = 0;
int plato_clone = 3;
int blast_door = 0;
int killer_count = 0;

SDL_Surface* screen;

gfx_cursor cursor;
char gamePath[256];
char *location = "HQ";

char* gamePathInit(const char* path) 
{
    uintptr_t i, j;
    
    for(i = 0, j = 0; path[i] != '\0'; i++) 
    {
        if((path[i] == '\\') || (path[i] == '/')) j = i + 1;
    }
    
    strncpy(gamePath, path, j);
    
    return gamePath;
}

void print_text(char *text)
{
    int height = renderer_font_height();
    
    renderer_font_print(&cursor, text);
    
    if((cursor.y + height) > (display_height - (2 * height))) 
    {
        more();
    }
}

void print_options(char *left, char *right)
{
   int width = renderer_font_width(right); 
   int height = renderer_font_height();
   
   gfx_cursor left_cur = { 2, display_height - height - 2 };
   gfx_cursor right_cur = { display_width - width - 2, display_height - height - 2 };
   
   renderer_fill_rect(0, display_height - height - 4, display_width, height + 4, 0xAA, 0x37, 0x00);
   renderer_font_print(&left_cur, left);
   renderer_font_print(&right_cur, right);
}

char get_char()
{
    SDL_Event event;
    
    while(1)
    {
        while(!SDL_PollEvent(&event)); // Wait for keypress

        if(event.type != SDL_KEYDOWN) continue; // Wait for they key to be pressed

#ifdef ENABLE_SCREENSHOT
        if(event.key.keysym.sym == SDLK_BACKSPACE)
        {
            renderer_screenshot();
        }
#endif
        
        if(event.key.keysym.sym == SDLK_ESCAPE)
        {
            renderer_release();
            SDL_Quit();
            exit(0);
        }

        if(event.key.keysym.sym == SDLK_LCTRL)  return 'A';
        if(event.key.keysym.sym == SDLK_LALT)   return 'B';
        if(event.key.keysym.sym == SDLK_SPACE)  return 'X';
        if(event.key.keysym.sym == SDLK_LSHIFT) return 'Y';

        // Didn't recognize the input
    }
}

void clear()
{
    int height = renderer_font_height();
    gfx_cursor title = {136 , 2};
    char text[256];
    
    cursor.x = 0;
    cursor.y = renderer_font_height() + 8;
    
    renderer_clear(0, 0, 0);
    renderer_fill_rect(0, 0, display_width, renderer_font_height() + 4, 0x00, 0x00, 0x66);
    renderer_font_print(&title, "PARANOIA");
    
    if(intro) return;
    
    sprintf(text, "%s", (ultra_violet ? "Ultraviolet" : "Red"));
    title.x = display_width - renderer_font_width(text) - 2;
    renderer_font_print(&title, text);
    
    sprintf(text, "%s", location);
    title.x = 2;
    renderer_font_print(&title, text);
}

void more()
{
    print_options("Exit (Select)", "More...");
        
#ifdef DEBUG
    printf("(page %d)",page);
#endif
    
    if(get_char() == 'X')
    {
        clear();
        character();
        more();
    }
    else
    {
        clear();
    }
}

int new_clone(int resume)
{
    char text[512];
    
    sprintf(text, "\nClone %d just died.\n\n", clone);
    print_text(text);

    if(++clone>6)
    {
        print_text("\n*** You Lose ***\n\nAll your clones are dead. Your name has been stricken from the records.\n\n			THE END\n");
        
        return 0;
    }
    else
    {
        sprintf(text, "Clone %d now activated.\n", clone);
        print_text(text);
        
        ultra_violet = 0;
        action_doll = 0;
        hit_points = 10;
        killer_count = 0;
        location = "HQ";
        
        more();
        
        return resume;
    }
}

int dice_roll(int number, int faces)
{
	int i, total=0;
        
	for(i=number; i>0; i--) total += rand()%faces + 1;
        
	return total;
}

void instructions()
{
	print_text("Welcome to Paranoia!\n\n\n");
	print_text("HOW TO PLAY:\n\n");
	print_text("  Just press a button until you are asked to make a \n");
	print_text("  choice.\n\n");
	print_text("  Press 'A' or 'B' or whatever for your choice.\n\n");
	print_text("  You may press 'X' at any time to get a display of \n");
	print_text("  your statistics.\n\n");
	print_text("  Always choose the least dangerous option. Continue\n");
	print_text("  doing this until you win.\n\n");
	print_text("  At times you will use a skill or engage in combat \n");
	print_text("  and and will be informed of the outcome. These\n");
	print_text("  sections will be self explanatory.");
        more();
        
	print_text("HOW TO DIE:\n\n");
	print_text("  As Philo-R-DMD you will die at times during the \n");
	print_text("  adventure. When this happens you will be given \n");
	print_text("  an new clone at a particular location.\n\n");
	print_text("  The new Philo-R will usually have to retrace some\n");
	print_text("  of the old Philo-R\'s path; hopefully he won\'t\n");
	print_text("  make the same mistake as his predecessor.\n\n");
	print_text("HOW TO WIN:\n\n");
	print_text("  Simply complete the mission before you expend all\n");
	print_text("  six clones.\n\n");
	print_text("  If you make it, congratulations.\n\n");
	print_text("  If not, you can try again later.");
}

void character()
{
    char player[40];
    
    print_text("=====================================================\n");
    sprintf(player, "The Character : Philo-R-DMD %d\n\n", clone);
    print_text(player);
    print_text("Primary Attributes        Secondary Attributes\n");
    print_text("=====================================================\n");
    print_text("Strength ............ 13  Carrying Capacity ...... 30\n");
    print_text("Endurance ........... 13  Damage Bonus ............ 0\n");
    print_text("Agility ............. 15  Macho Bonus ............ -1\n");
    print_text("Manual Dexterity .... 15  Melee Bonus ........... +5%\n");
    print_text("Moxie ............... 13  Aimed Weapon Bonus ... +10%\n");
    print_text("Chutzpah ............. 8  Comprehension Bonus ... +4%\n");
    print_text("Mechanical Aptitude . 14  Believability Bonus ... +5%\n");
    print_text("Power Index ......... 10  Repair Bonus .......... +5%\n");
    print_text("=====================================================\n");
    more();
    
    print_text("=====================================================\n");
    sprintf(player, "The Character : Philo-R-DMD %d\n\n", clone);
    print_text(player);
    print_text("=====================================================\n");
    print_text("Secret Society: Illuminati               Credits: 160\n");
    print_text("Secret Society Rank: 1      Service Group: Power Svcs\n");
    print_text("Mutant Power: Precognition\n");
    print_text("Weapon: laser pistol; to hit, 40%; type, L; \n");
    print_text("        Range, 50m; Reload, 6r; Malfnt, 00\n");
    print_text("Skills: Basics 1(20%), Aimed Weapon Combat 2(35%),\n");
    print_text("        Laser 3(40%), Personal Development 1(20%), \n");
    print_text("        Communications 2(29%), Intimidation 3(34%)\n");
    print_text("Equipment: Red Reflec Armour, Laser Pistol,\n");
    print_text("           Laser Barrel (red), Notebook & Stylus,\n");
    print_text("           Knife, Com Unit 1, Jump suit, Secret \n");
    print_text("           Illuminati Eye-In-The-Pyramid(tm) Decoder\n");
    print_text("           ring, Utility Belt & Pouches\n");
    print_text("=====================================================\n");
}

int choose(int a, char *aptr, int b, char *bptr)
{
    char choices[512];
    char button;

    while(1)
    {
        renderer_fill_rect(0, cursor.y + 4, display_width, 1, 0x88, 0x88, 0x88);
        
        cursor.y += 7;
        
        sprintf(choices, " A - %s.\n B - %s.", aptr, bptr);
        print_text(choices);

        print_options("Exit (Select)", "Choose (A / B)");   

        while(1)
        {
            button = get_char();

            if(button == 'X')
            {
                clear();
                character();
                more();
                continue;
            }

            if(button == 'A') return a;
            if(button == 'B') return b;
        }
    }
}

int choose3(int a, char *aptr, int b, char *bptr, int y, char *yptr)
{
    char choices[1024];
    char button;
    
    while(1)
    {
        renderer_fill_rect(0, cursor.y + 4, display_width, 1, 0x88, 0x88, 0x88);
        
        cursor.y += 7;
        
        sprintf(choices, " A - %s.\n B - %s.\n Y - %s.", aptr, bptr, yptr);
        print_text(choices);

        print_options("Exit (Select)", "Choose (A / B / Y)");   

        button = get_char();

        if(button == 'X')
        {
            clear();
            character();
            more();
            continue;
        }

        switch(button)
        {
            case 'A': return (a < 0) ? new_clone(-a) : a;
            case 'B': return b;
            case 'Y': return y;
        }
    }
}

int page1()
{
    print_text("You wake up face down on the red and pink checked E-Z-Kleen linoleum floor.\n\n");
    print_text("You recognize the pattern, it\'s the type preferred in the internal security briefing cells.\n\n");
    print_text("When you finally look around you, you see that you are alone ");
    print_text("in a large mission briefing room.\n");
    
    intro = 0;
    
    more();
    
    return 57;
}

int page2()
{
	print_text("\"Greetings,\" says the kindly Internal Security self incrimination expert who ");
	print_text("meets you at the door, \"How are we doing today?\" \n\nHe offers you a donut ");
	print_text("and coffee and asks what brings you here. This doesn\'t seem so bad, so you ");
	print_text("tell him that you have come to confess some possible security lapses.  He ");
	print_text("smiles knowingly, deftly catching your coffee as you slump to the floor. ");
	print_text("\"Nothing to be alarmed about; it\'s just the truth serum,\" he says, ");
	print_text("dragging you back into a discussion room.\n\n");
	print_text("The next five hours are a dim haze, but you can recall snatches of conversation ");
	print_text("about your secret society, your mutant power, and your somewhat paranoid ");
	print_text("distrust of The Computer. This should explain why you are hogtied and moving ");
	print_text("slowly down the conveyer belt towards the meat processing unit in Food ");
	print_text("Services.\n");
        more();
        
	if (computer_request==1) return new_clone(45);
	else 			 return new_clone(32);
}

int page3()
{
	print_text("You walk to the nearest Computer terminal and request more information about ");
	print_text("Christmas. The Computer says, \"That is an A-1 ULTRAVIOLET ONLY IMMEDIATE ");
	print_text("TERMINATION classified topic. \n\nWhat is your clearance please, Troubleshooter?\"\n");
        
	return choose(
                4, "You give your correct clearance", 
                5, "You lie and claim Ultraviolet clearance");
}

int page4()
{
	print_text("\"That is classified information, Troubleshooter, thank you for your inquiry.\n\n");
	print_text("Please report to an Internal Security self incrimination station as soon as ");
	print_text("possible.\"\n");
        more();
        
	return 9;
}

int page5()
{
	print_text("The computer says, \"Troubleshooter, you are not wearing the correct color ");
	print_text("uniform. You must put on an Ultraviolet uniform immediately.  I have seen to ");
	print_text("your needs and ordered one already; it will be here shortly.  Please wait with ");
	print_text("your back to the wall until it arrives.\" \n\nIn less than a minute an infrared ");
	print_text("arrives carrying a white bundle. He asks you to sign for it, then hands it to ");
	print_text("you and stands back, well outside of a fragmentation grenade\'s blast radius.\n");
        
	return choose(
                6, "You open the package and put on the uniform", 
                7, "You finally come to your senses and run for it");
}

int page6()
{
	print_text("The uniform definitely makes you look snappy and pert. It really looks ");
	print_text("impressive, and even has the new lopsided lapel fashion that you admire so ");
	print_text("much. \n\nWhat\'s more, citizens of all ranks come to obsequious attention as you ");
	print_text("walk past. This isn\'t so bad being an Ultraviolet. You could probably come ");
	print_text("to like it, given time.\n\n");
	print_text("The beeping computer terminal interrupts your musings.\n");
        more();
        
	ultra_violet = 1;
        
	return 8;
}

int page7()
{
	print_text("The corridor lights dim and are replaced by red battle lamps as the Security ");
	print_text("Breach alarms howl all around you. You run headlong down the corridor and ");
	print_text("desperately windmill around a corner, only to collide with a squad of 12 Blue ");
	print_text("clearance Vulture squadron soldiers. \"Stop, Slime Face,\" shouts the ");
	print_text("commander, \"or there won\'t be enough of you left for a tissue sample.\"\n\n");
	print_text("\"All right, soldiers, stuff the greasy traitor into the uniform,\" he orders, ");
	print_text("waving the business end of his blue laser scant inches from your nose. ");
	print_text("With his other hand he shakes open a white bundle to reveal a pristine new ");
	print_text("Ultraviolet citizen's uniform.\n\n");
        more();
	print_text("One of the Vulture squadron Troubleshooters grabs you by the neck in the ");
	print_text("exotic and very painful Vulture Clamp(tm) death grip (you saw a special about ");
	print_text("it on the Teela O\'Malley show), while the rest tear off your clothes and ");
	print_text("force you into the Ultraviolet uniform. The moment you are dressed they step ");
	print_text("clear and stand at attention.\n\n");
	print_text("\"Thank you for your cooperation, sir,\" says the steely eyed leader of the ");
	print_text("Vulture Squad. \"We will be going about our business now.\" With perfect ");
	print_text("timing the Vultures wheel smartly and goosestep down the corridor.\n\n");
        more();
	print_text("Special Note: don\'t make the mistake of assuming that your skills have ");
	print_text("improved any because of the uniform; you\'re only a Red Troubleshooter ");
	print_text("traitorously posing as an Ultraviolet, and don\'t you forget it!\n\n");
	print_text("Suddenly, a computer terminal comes to life beside you.\n\n");
        more();
        
	ultra_violet=1;
        
	return 8;
}

int page8()
{
	print_text("\"Now, about your question, citizen. Christmas was an old world marketing ploy ");
	print_text("to induce lower clearance citizens to purchase vast quantities of goods, thus ");
	print_text("accumulation a large amount of credit under the control of a single class of ");
	print_text("citizen known as Retailers. \n\nThe strategy used is to imply that all good ");
	print_text("citizens give gifts during Christmas, thus if one wishes to be a valuable ");
	print_text("member of society one must also give gifts during Christmas.  More valuable ");
	print_text("gifts make one a more valuable member, and thus did the Retailers come to ");
	print_text("control a disproportionate amount of the currency. \n\nIn this way Christmas ");
	print_text("eventually caused the collapse of the old world.  Understandably, Christmas ");
	print_text("has been declared a treasonable practice in Alpha Complex.\n\n");
	print_text("Thank you for your inquiry.\"\n\n");
	print_text("You continue on your way to GDH7-beta.\n");
        more();
        
	return 10;
}

int page9()
{
	int choice;
	print_text("As you walk toward the tubecar that will take you to GDH7-beta, you pass one ");
	print_text("of the bright blue and orange Internal Security self incrimination stations. ");
	print_text("Inside, you can see an IS agent cheerfully greet an infrared citizen and then ");
	print_text("lead him at gunpoint into one of the rubber lined discussion rooms.\n\n");
        
	choice = choose(
                2, "You decide to stop here and chat, as ordered\n     by The Computer", 
                10, "You just continue blithely on past");
        
	if(choice == 2) computer_request = 1;
	else	        computer_request = 0;
        
	return choice;
}

int page10()
{
	int choice;
        
	print_text("You stroll briskly down the corridor, up a ladder, across an unrailed catwalk, ");
	print_text("under a perilously swinging blast door in urgent need of repair, and into ");
	print_text("tubecar grand central. \n\nThis is the bustling hub of Alpha Complex tubecar ");
	print_text("transportation. Before you spreads a spaghetti maze of magnalift tube tracks ");
	print_text("and linear accelerators. \n\nYou bravely study the specially enhanced 3-D tube ");
	print_text("route map; you wouldn\'t be the first Troubleshooter to take a fast tube ride ");
	print_text("to nowhere.\n\n");
        
        if(ultra_violet == 0)
	{
            choice = choose(
                    3, "You decide to ask The Computer about Christmas \n     using a nearby terminal", 
                    10, "You think you have the route worked out, so \n     you\'ll board a tube train");

            if(choice == 3) return choice;
            
            clear();
	}
        else
        {
            more();
        }
        
	print_text("You nervously select a tubecar and step aboard.\n\n");
        
	if(dice_roll(2,10) < MOXIE)
	{
            print_text("You just caught a purple line tubecar.\n");
            more();

            return 13;
        }
	else
	{
            print_text("You just caught a brown line tubecar.\n");
            more();

            return 48;
	}
}

int page11()
{
	print_text("The printing on the folder says \"Experimental Self Briefing.\"\n\n");
	print_text("You open it and begin to read the following:\n\n");
	print_text("Step 1: Compel the briefing subject to attend the\n");
	print_text("        briefing.\n\n");
	print_text("    Note: See Experimental Briefing Sub Form\n");
	print_text("    Indigo-WY-2, \'Experimental Self Briefing\n");
	print_text("    Subject Acquisition Through The Use Of\n");
	print_text("    Neurotoxin Room Foggers.\'\n\n");
        more();
	print_text("Step 2: Inform the briefing subject that the \n");
	print_text("        briefing has begun.\n\n");
	print_text("    ATTENTION: THE BRIEFING HAS BEGUN.\n\n");
        more();
	print_text("Step 3: Present the briefing material to the briefing\n");
	print_text("        subject.\n\n");
	print_text("    GREETINGS TROUBLESHOOTER.\n");
	print_text("    YOU HAVE BEEN SPECIALLY SELECTED TO\n");
	print_text("    SINGLEHANDEDLY WIPE OUT A DEN OF TRAITOROUS\n");
	print_text("    CHRISTMAS ACTIVITY. YOUR MISSION IS TO GO TO\n");
	print_text("    GOODS DISTRIBUTION HALL 7-BETA AND ASSESS ANY\n");
	print_text("    CHRISTMAS ACTIVITY YOU FIND THERE. YOU ARE TO\n");
	print_text("    INFILTRATE THESE CHRISTMAS CELEBRANTS, LOCATE\n");
	print_text("    THEIR RINGLEADER, AN UNKNOWN MASTER RETAILER,\n");
	print_text("    AND BRING HIM BACK FOR EXECUTION AND TRIAL.\n");
	print_text("    THANK YOU. THE COMPUTER IS YOUR FRIEND.\n\n");
        more();
	print_text("Step 4: Sign the briefing subject\'s briefing release\n");
	print_text("        form to indicate that the briefing subject\n");
	print_text("        has completed the briefing.\n\n");
	print_text("    ATTENTION: PLEASE SIGN YOUR BRIEFING RELEASE\n");
	print_text("               FORM.\n\n");
        more();
	print_text("Step 5: Terminate the briefing\n\n");
	print_text("    ATTENTION: THE BRIEFING IS TERMINATED.\n\n");
	more();
	print_text("You walk to the door and hold your signed briefing release form up to the ");
	print_text("plexiglass window.\n\nA guard scrutinizes it for a moment and then slides back ");
	print_text("the megabolts holding the door shut. \n\nYou are now free to continue the ");
	print_text("mission.\n");
        
	return choose(
                3, "You wish to ask The Computer for more\n     information about Christmas", 
                10, "You have decided to go directly to Goods\n     Distribution Hall 7-beta");
}

int page12()
{
	print_text("You walk up to the door and push the button labeled \"push to exit.\"\n\n");
	print_text("Within seconds a surly looking guard shoves his face into the small plexiglass ");
	print_text("window.  You can see his mouth forming words but you can\'t hear any of them.\n\n");
	print_text("You just stare at him blankly for a few moments until he points down to a ");
	print_text("speaker on your side of the door. When you put your ear to it you can barely ");
	print_text("hear him say, \"Let\'s see your briefing release form, bud. You aren\'t ");
	print_text("getting out of here without it.\"\n");
        
	return choose(
                11, "You sit down at the table and read the \n     Orange packet", 
                57, "You stare around the room some more");
}

int page13()
{
	print_text("You step into the shiny plasteel tubecar, wondering why the shape has always ");
	print_text("reminded you of bullets. \n\nThe car shoots forward the instant your feet touch ");
	print_text("the slippery gray floor, pinning you immobile against the back wall as the ");
	print_text("tubecar careens toward GDH7-beta. \n\nYour only solace is the knowledge that it ");
	print_text("could be worse, much worse.\n\n");
	print_text("Before too long the car comes to a stop. You can see signs for GDH7-beta ");
	print_text("through the window. \n\nWith a little practice you discover that you can crawl ");
	print_text("to the door and pull open the latch.\n");
        more();
        
	return 14;
}

int page14()
{
	print_text("You manage to pull yourself out of the tubecar and look around. \n\nBefore you is ");
	print_text("one of the most confusing things you have ever seen, a hallway that is ");
	print_text("simultaneously both red and green clearance. \n\nIf this is the result of ");
	print_text("Christmas then it\'s easy to see the evils inherent in its practice.\n\n");
	print_text("You are in the heart of a large goods distribution center. You can see all ");
	print_text("about you evidence of traitorous secret society Christmas celebration; rubber ");
	print_text("faced robots whiz back and forth selling toys to holiday shoppers, simul-plast ");
	print_text("wreaths hang from every light fixture, while ahead in the shadows is a citizen ");
	print_text("wearing a huge red synthetic flower.\n");
        more();
        
        location = "GDH7-beta";
        
	return 22;
}

int page15()
{
	print_text("You are set upon by a runty robot with a queer looking face and two pointy ");
	print_text("rubber ears poking from beneath a tattered cap. \n\n\"Hey mister,\" it says, ");
	print_text("\"you done all your last minute Christmas shopping? I got some real neat junk ");
	print_text("here. You don\'t wanna miss the big day tommorrow, if you know what I mean.\"\n\n");
	print_text("The robot opens its bag to show you a pile of shoddy Troubleshooter dolls. It ");
	print_text("reaches in and pulls out one of them. \n\n");
        more();
	print_text("\"Look, these Action Troubleshooter(tm) ");
	print_text("dolls are the neatest thing.  This one\'s got moveable arms and when you ");
	print_text("squeeze him, his little rifle squirts realistic looking napalm.  It\'s only ");
	print_text("50 credits. Oh yeah, Merry Christmas.\"\n");
        
        return choose3(
                16, "You decide to buy the doll", 
                17, "You shoot the robot", 
                22, "You ignore the robot and keep searching");
}

int page16()
{
	print_text("The doll is a good buy for fifty credits; it will make a fine Christmas present ");
	print_text("for one of your friends. After the sale the robot rolls away. \n\nYou can use ");
	print_text("the doll later in combat. It works just like a cone rifle firing napalm, ");
	print_text("except that occasionally it will explode and blow the user to smithereens.\n\n");
	print_text("But don\'t let that stop you.\n");
        more();
        
	action_doll = 1;
        
	return 22;
}

int page17()
{
    int i, robot_hp = 15;
    
    print_text("You whip out your laser and shoot the robot, but not before it squeezes the ");
    print_text("toy at you. The squeeze toy has the same effect as a cone rifle firing napalm, ");
    print_text("and the elfbot\'s armour has no effect against your laser.\n\n");

    for(i=0;i<2;i++)
    {
        if(dice_roll(1,100)<=25)
        {
            print_text("You have been hit!\n\n");
            
            hit_points-= dice_roll(1,10);

            if(hit_points <= 0)
            {
                more();
                return new_clone(45);
            }
        }
        else 
        {
            print_text("It missed you, but not by much!\n\n");
        }

        if(dice_roll(1,100) <= 40)
        {
            print_text("You zapped the little bastard!\n\n");

            robot_hp-= dice_roll(2,10);

            if(robot_hp <= 0)
            {
                print_text("You wasted it! Good shooting!\n\n");
                print_text("You will need more evidence, so you search GDH7-beta further ");
                
                if(hit_points < 10) print_text("after the GDH medbot has patched you up.\n");
                
                hit_points=10;

                more();

                return 22;
            }
        }
        else 
        {
            print_text("Damn! You missed!\n");
        }
    };

    print_text("It tried to fire again, but the toy exploded and demolished it.\n\n");
    print_text("You will need more evidence, so you search GDH7-beta further ");

    if (hit_points < 10) print_text("after the GDH medbot has patched you up.\n");

    hit_points = 10;

    more();

    return 22;
}

int page18()
{
	print_text("You walk to the center of the hall, ogling like an infrared fresh from the ");
	print_text("clone vats. \n\nTowering before you is the most unearthly thing you have ever ");
	print_text("seen, a green multi armed mutant horror hulking 15 feet above your head. ");
	print_text("Its skeletal body is draped with hundreds of metallic strips (probably to ");
	print_text("negate the effects of some insidious mutant power), and the entire hideous ");
	print_text("creature is wrapped in a thousand blinking hazard lights.\n\n");
	print_text("It\'s times like this when you wish you\'d had some training for this job.  Luckily the ");
	print_text("creature doesn\'t take notice of you but stands unmoving, as though waiting for ");
	print_text("a summons from its dark lord, the Master Retailer.\n\n");
	print_text("WHAM, suddenly you are struck from behind.\n");
        more();
        
	if(dice_roll(2,10) < AGILITY)	return 19;
	else				return 20;
}

int page19()
{
	print_text("Quickly you regain your balance, whirl and fire your laser into the Ultraviolet ");
	print_text("citizen behind you. For a moment your heart leaps to your throat, then you ");
	print_text("realize that he is indeed dead and you will be the only one filing a report on ");
	print_text("this incident.\n\n");
	print_text("Besides, he was participating in this traitorous Christmas ");
	print_text("shopping, as is evident from the rain of shoddy toys falling all around you.\n\n");
	print_text("Another valorous deed done in the service of The Computer!\n\n");
        
	if(++killer_count > (MAXKILL-clone))
        {
            more();
            return 21;
        }
        
	if(read_letter==1)
        {
            more();
            return 22;
        }
        
	return choose(
                34, "You search the body, keeping an eye open for \n     Internal Security", 
                22, "You run away like the cowardly dog you are");
}

int page20()
{
	print_text("Oh no! you can\'t keep your balance. You\'re falling, falling head first into ");
	print_text("the Christmas beast\'s gaping maw. It\'s a valiant struggle; you think you are ");
	print_text("gone when its poisonous needles dig into your flesh, but with a heroic effort ");
	print_text("you jerk a string of lights free and jam the live wires into the creature\'s ");
	print_text("spine.\n\n");
	print_text("The Christmas beast topples to the ground and begins to burn, filling ");
	print_text("the area with a thick acrid smoke.  It takes only a moment to compose yourself, ");
	print_text("and then you are ready to continue your search for the Master Retailer.\n");
        more();
        
	return 22;
}

int page21()
{
	print_text("You have been wasting the leading citizens of Alpha Complex at a prodigious ");
	print_text("rate.  This has not gone unnoticed by the Internal Security squad at GDH7-beta.\n\n");
	print_text("Suddenly, a net of laser beams spear out of the gloomy corners of the hall, ");
	print_text("chopping you into teeny, weeny bite size pieces.\n");
        more();
        
	return new_clone(45);
}

int page22()
{
    cursor.y = 12 * 8;
    cursor.x = 14 * 6;
    print_text("You are searching Goods");

    cursor.y = 13 * 8 + 2;
    cursor.x = 14 * 6 - 2;
    print_text("Distribution Hall 7-beta");
    more();

    switch(dice_roll(1,4))
    {
        case 1:	return 18;
        case 2: return 15;
        case 3: return 18;
        case 4: return 29;
    }
}

int page23()
{
	print_text("You go to the nearest computer terminal and declare yourself a mutant.\n\n");
	print_text("\"A mutant, he\'s a mutant,\" yells a previously unnoticed infrared who had ");
	print_text("been looking over your shoulder.  You easily gun him down, but not before a ");
	print_text("dozen more citizens take notice and aim their weapons at you.\n");
        
	return choose(
                28, "You tell them that it was really only a\n     bad joke", 
                24, "You want to fight it out, one against twelve");
}

int page24()
{
	print_text("Golly, I never expected someone to pick this. I haven\'t even designed ");
	print_text("the 12 citizens who are going to make a sponge out of you. \n\nTell you what, ");
	print_text("I\'ll give you a second chance.\n");
        
	return choose(
                28, "You change your mind and say it was only a\n     bad joke", 
                25, "You REALLY want to shoot it out");
}

int page25()
{
	print_text("Boy, you really can\'t take a hint!\n\n");
	print_text("They\'re closing in.  Their trigger fingers are twitching, they\'re about to ");
	print_text("shoot.  This is your last chance.\n");
        
	return choose(
                28, "You tell them it was all just a bad joke" , 
                26, "You are going to shoot");
}

int page26()
{
	print_text("You can read the cold, sober hatred in their eyes (They really didn\'t think ");
	print_text("it was funny), as they tighten the circle around you. \n\nOne of them shoves a ");
	print_text("blaster up your nose, but that doesn\'t hurt as much as the multi-gigawatt ");
	print_text("carbonium tipped food drill in the small of your back.\n\n");
	print_text("You spend the remaining micro-seconds of your life wondering what you did wrong\n");
        more();
        
	return new_clone(32);
}

int page27()
{
	/* doesn't exist.  Can't happen with computer version.
	   designed to catch dice cheats */
}

int page28()
{
	print_text("They don\'t think it\'s funny.\n");
        more();
        
	return 26;
}

int page29()
{
	print_text("\"Psst, hey citizen, come here.  Pssfft,\" you hear. \n\nWhen you peer around ");
	print_text("you can see someone\'s dim outline in the shadows. \"I got some information ");
	print_text("on the Master Retailer. It\'ll only cost you 30 psst credits.\"\n");
        
        return choose3(
                30, "You pay the 30 credits for the info.", 
                31, "You would rather threaten him for the\n     information.", 
                22, "You ignore him and walk away.");
}

int page30()
{
	print_text("You step into the shadows and offer the man a thirty credit bill. \"Just drop ");
	print_text("it on the floor,\" he says. \n\n\"So you\'re looking for the Master Retailer, pssfft? ");
	print_text("I\'ve seen him, he\'s a fat man in a fuzzy red and white jump suit.  They say ");
	print_text("he\'s a high programmer with no respect for proper security.  If you want to ");
	print_text("find him then pssfft step behind me and go through the door.\"\n\n");
	print_text("Behind the man is a reinforced plasteel blast door.  The center of it has been ");
	print_text("buckled toward you in a manner you only saw once before when you were field ");
	print_text("testing the rocket assist plasma slingshot (you found it easily portable but ");
	print_text("prone to misfire).  Luckily it isn\'t buckled too far for you to make out the ");
	print_text("warning sign. \n\n");
        more();
        print_text("WARNING!! Don\'t open this door or the same thing will happen to ");
	print_text("you.  Opening this door is a capital offense.  Do not do it.  Not at all. This ");
	print_text("is not a joke.\n");
        
        return choose3(
                56, "You use your Precognition mutant power on\n     opening the door", 
                33, "You just go through the door anyway", 
                22, "You decide it\'s too dangerous and walk away");
}

int page31()
{
	print_text("Like any good troubleshooter you make the least expensive decision and threaten ");
	print_text("him for information.  With lightning like reflexes you whip out your laser and ");
	print_text("stick it up his nose.  \"Talk, you traitorous Christmas celebrator, or who nose ");
	print_text("what will happen to you, yuk yuk,\" you pun menacingly, and then you notice ");
	print_text("something is very wrong.  He doesn\'t have a nose.  As a matter of fact he\'s ");
	print_text("made of one eighth inch cardboard and your laser is sticking through the other ");
	print_text("side of his head.  \"Are you going to pay?\" says his mouth speaker, ");
	print_text("\"or are you going to pssfft go away stupid?\"\n");
        
	return choose(
                30, "You pay the 30 credits", 
                22, "You pssfft go away stupid");
}

int page32()
{
	print_text("Finally it\'s your big chance to prove that you\'re as good a troubleshooter ");
	print_text("as your previous clone. \n\nYou walk briskly to mission briefing and pick up your ");
	print_text("previous clone\'s personal effects and notepad. After reviewing the notes you ");
	print_text("know what has to be done. \n\nYou catch the purple line to Goods Distribution Hall ");
	print_text("7-beta and begin to search for the blast door.\n");
        more();
        
        location = "GDH7-beta";
        
	return 22;
}

int page33()
{
	blast_door=1;
	print_text("You release the megabolts on the blast door, then strain against it with your ");
	print_text("awesome strength. \n\nSlowly the door creaks open. \n\nYou bravely leap through the ");
	print_text("opening and smack your head into the barrel of a 300 mm \'ultra shock\' class ");
	print_text("plasma cannon. \n\nIt\'s dark in the barrel now, but just before your head got ");
	print_text("stuck you can remember seeing a group of technicians anxiously watch you leap ");
	print_text("into the room.\n");
        more();
        
	if(ultra_violet == 1)	return 35;
	else			return 36;
}

int page34()
{
	print_text("You have found a sealed envelope on the body.  You open it and read:\n\n\n\n");
	print_text("WARNING: Ultraviolet Clearance ONLY.    DO NOT READ.\n\n");
	print_text("Memo from Chico-U-MRX4 to Harpo-U-MRX5.\n\n");
	print_text("The planned takeover of the Troubleshooter Training Course goes well, Comrade. ");
	print_text("Once we have trained the unwitting bourgeois troubleshooters to work as ");
	print_text("communist dupes, the overthrow of Alpha Complex will be unstoppable. \n\nMy survey ");
	print_text("of the complex has convinced me that no one suspects a thing; soon it will be ");
	print_text("too late for them to oppose the revolution.\n\n");
	more();
	print_text("The only thing that could possibly ");
	print_text("impede the people\'s revolution would be someone alerting The Computer to our ");
	print_text("plans (for instance, some enterprising Troubleshooter could tell The Computer ");
	print_text("that the communists have liberated the Troubleshooter Training Course and plan ");
	print_text("to use it as a jumping off point from which to undermine the stability of all ");
	print_text("Alpha Complex), but as we both know, the capitalistic Troubleshooters would ");
	print_text("never serve the interests of the proletariat above their own bourgeois desires.\n\n");
	print_text("P.S. I\'m doing some Christmas shopping later today.  Would you like me to pick ");
	print_text("you up something?\n");
	more();
	print_text("When you put down the memo you are overcome by that strange deja\'vu again.\n\n");
	print_text("You see yourself talking privately with The Computer.\n\nYou are telling it all ");
	print_text("about the communists\' plan, and then the scene shifts and you see yourself ");
	print_text("showered with awards for foiling the insidious communist plot to take over the ");
	print_text("complex.\n");
        
	read_letter=1;
        
	return choose(
                46, "You rush off to the nearest computer terminal\n     to expose the commies", 
                22, "You wander off to look for more evidence");
}

int page35()
{
	print_text("\"Oh master,\" you hear through the gun barrel, \"where have you been? It is ");
	print_text("time for the great Christmas gifting ceremony.  You had better hurry and get ");
	print_text("the costume on or the trainee may begin to suspect.\"  For the second time ");
	print_text("today you are forced to wear attire not of your own choosing.  They zip the ");
	print_text("suit to your chin just as you hear gunfire erupt behind you.\n\n");
	print_text("\"Oh no! Who left the door open?  The commies will get in.  Quick, fire the ");
	print_text("laser cannon or we\'re all doomed.\"\n\n");
	print_text("\"Too late you capitalist swine, the people\'s revolutionary strike force claims ");
	print_text("this cannon for the proletariat\'s valiant struggle against oppression.  Take ");
	print_text("that, you running dog imperialist lackey.  ZAP, KAPOW\"\n\n");
        more();
	print_text("Just when you think that things couldn\'t get worse, \"Aha, look what we have ");
	print_text("here, the Master Retailer himself with his head caught in his own cannon.  His ");
	print_text("death will serve as a symbol of freedom for all Alpha Complex.\n\n");
	print_text("Fire the cannon.\"\n");
        more();
        
	return new_clone(32);
}

int page36()
{
	print_text("\"Congratulations, troubleshooter, you have successfully found the lair of the ");
	print_text("Master Retailer and completed the Troubleshooter Training Course test mission,\" ");
	print_text("a muffled voice tells you through the barrel.\n\n");
        more();
	print_text("\"Once we dislodge your head ");
	print_text("from the barrel of the \'Ultra Shock\' plasma cannon you can begin with the ");
	print_text("training seminars, the first of which will concern the 100% accurate ");
	print_text("identification and elimination of unregistered mutants. \n\nIf you have any ");
	print_text("objections please voice them now.\"\n");
        
        return choose3(
                -32, "You appreciate his courtesy and voice\n     an objection.", 
                23, "After your head is removed from the cannon,\n     you register as a mutant.", 
                37, "After your head is removed from the cannon,\n     you go to the unregistered mutant identification\n     and elimination seminar.");
        
}

int page37()
{
	print_text("\"Come with me please, Troubleshooter,\" says the Green clearance technician ");
	print_text("after he has dislodged your head from the cannon. \n\n\"You have been participating ");
	print_text("in the Troubleshooter Training Course since you got off the tube car in ");
	print_text("GDH7-beta,\" he explains as he leads you down a corridor. \n\n\"The entire ");
	print_text("Christmas assignment was a test mission to assess your current level of ");
	print_text("training.  You didn\'t do so well.  We\'re going to start at the beginning with ");
	print_text("the other student.  Ah, here we are, the mutant identification and elimination ");
	print_text("lecture.\"\n\n");
        more();
        print_text("He shows you into a vast lecture hall filled with empty seats. ");
	print_text("There is only one other student here, a Troubleshooter near the front row ");
	print_text("playing with his Action Troubleshooter(tm) figure. \n\n\"Find a seat and I will ");
	print_text("begin,\" says the instructor.");
        more();
        
        location = "Lecture Hall";
        
	return 38;
}

int page38()
{
    char clone_text[256];
    
    sprintf(clone_text, "\"I am Plato-B-PHI%d, head of mutant propaganda here at the training course. ", plato_clone);
    
    print_text(clone_text);
    print_text("If you have any questions about mutants please come to me.  Today I will be ");
    print_text("talking about mutant detection.  \n\nDetecting mutants is very easy. One simply ");
    print_text("watches for certain tell tale signs, such as the green scaly skin, the third ");
    print_text("arm growing from the forehead, or other similar disfigurements so common with ");
    print_text("their kind. \n\nThere are, however, a few rare specimens that show no outward sign ");
    print_text("of their treason.  This has been a significant problem, so our researchers have ");
    print_text("been working on a solution. \n\nI would like a volunteer to test this device,\" ");
    print_text("he says, holding up a ray gun looking thing.");
    more();
    print_text("\"It is a mutant detection ray. ");
    print_text("This little button detects for mutants, and this big button stuns them once ");
    print_text("they are discovered.  Who would like to volunteer for a test?\"\n\n");
    print_text("The Troubleshooter down the front squirms deeper into his chair.\n");
    
    return choose(
            39, "You volunteer for the test", 
            40, "You duck behind a chair and hope the\n     instructor doesn\'t notice you");
}

int page39()
{
	print_text("You bravely volunteer to test the mutant detection gun. You stand up and walk ");
	print_text("down the steps to the podium, passing a very relieved Troubleshooter along the ");
	print_text("way. \n\nWhen you reach the podium Plato-B-PHI hands you the mutant detection gun ");
	print_text("and says, \"Here, aim the gun at that Troubleshooter and push the small button. ");
	print_text("If you see a purple light, stun him.\" \n\nGrasping the opportunity to prove your ");
	print_text("worth to The Computer, you fire the mutant detection ray at the Troubleshooter. ");
	print_text("A brilliant purple nimbus instantly surrounds his body. You slip your finger ");
	print_text("to the large stun button and he falls writhing to the floor.\n\n");
        more();
	print_text("\"Good shot,\" says the instructor as you hand him the mutant detection gun, ");
	print_text("\"I\'ll see that you get a commendation for this.  It seems you have the hang ");
	print_text("of mutant detection and elimination.  You can go on to the secret society ");
	print_text("infiltration class.  I\'ll see that the little mutie gets packaged for ");
	print_text("tomorrow\'s mutant dissection class.\"\n");
        more();
        
	return 41;
}

int page40()
{
    print_text("You breathe a sigh of relief as Plato-B-PHI picks on the other Troubleshooter.\n\n");
    print_text("\"You down here in the front,\" says the instructor pointing at the other ");
    print_text("Troubleshooter, \"you\'ll make a good volunteer.  Please step forward.\"\n\n");
    print_text("The Troubleshooter looks around with a \'who me?\' expression on his face, but ");
    print_text("since he is the only one visible in the audience he figures his number is up. ");
    print_text("He walks down to the podium clutching his Action Troubleshooter(tm) doll before ");
    print_text("him like a weapon. \n\n\"Here,\" says Plato-B-PHI, \"take the mutant detection ray ");
    print_text("and point it at the audience.  If there are any mutants out there we\'ll know ");
    print_text("soon enough.\" \n\nSuddenly your skin prickles with static electricity as a bright ");
    print_text("purple nimbus surrounds your body.");
    more();
    print_text("\"Ha Ha, got one,\" says the instructor. ");
    print_text("\"Stun him before he gets away.\"");
    more();

    while(1)
    {
        if(dice_roll(1,100)<=30)
        {
            print_text("His shot hits you. You feel numb all over.\n\n");
            more();
            
            return 49;
        }
        else
        {
            print_text("His shot just missed.\n\n");
        }

        if(dice_roll(1,100)<=40)
        {
            print_text("You just blew his head off. His lifeless hand drops the mutant detector ray.\n\n");
            more();
            
            return 50;
        }
        else
        {
            print_text("You burnt a hole in the podium. He sights the mutant detector ray on you.\n\n");
        }
    }

    more();
}

int page41()
{
	print_text("You stumble down the hallway of the Troubleshooter Training Course looking for ");
	print_text("your next class.  Up ahead you see one of the instructors waving to you. \n\nWhen ");
	print_text("you get there he shakes your hand and says, \"I am Jung-I-PSY. Welcome to the ");
	print_text("secret society infiltration seminar. I hope you ...\"\n\n");
	print_text("You don\'t catch the rest of his greeting because \nyou\'re paying too much attention to his handshake; ");
	print_text("it is the strangest thing that has ever been done to your hand, sort of how it ");
	print_text("would feel if you put a neuro-whip in a high energy palm massage unit.\n\n");
        more();
	print_text("It doesn\'t take you long to learn what he is up to; you feel him briefly shake ");
	print_text("your hand with the secret Illuminati handshake.\n");
        
	return choose(
                42, "You respond with the proper Illuminati code\n     phrase, \"Ewige Blumenkraft\"", 
                43, "You ignore this secret society contact");
}

int page42()
{
	print_text("\"Aha, so you are a member of the elitist Illuminati secret society,\" he says ");
	print_text("loudly, \"that is most interesting.\"\n\n");
	print_text("He turns to the large class already ");
	print_text("seated in the auditorium and says, \"You see, class, by simply using the correct ");
	print_text("hand shake you can identify the member of any secret society.  Please keep your ");
	print_text("weapons trained on him while I call a guard.\n");
        
	return choose(
                51, "You run for it", 
                52, "You wait for the guard");
}

int page43()
{
	print_text("You sit through a long lecture on how to recognize and infiltrate secret ");
	print_text("societies, with an emphasis on mimicking secret handshakes.  The basic theory, ");
	print_text("which you realize to be sound from your Iluminati training, is that with the ");
	print_text("proper handshake you can pass unnoticed in any secret society gathering.\n\n");
	print_text("What\'s more, the proper handshake will open doors faster than an \'ultra shock\' ");
	print_text("plasma cannon. You are certain that with the information you learn here you ");
	print_text("will easily be promoted to the next level of your Illuminati secret society.\n\n");
	print_text("The lecture continues for three hours, during which you have the opportunity ");
	print_text("to practice many different handshakes. Afterwards everyone is directed to ");
	print_text("attend the graduation ceremony.\n\n");
        more();
	print_text("Before you must go you have a little time to ");
	print_text("talk to The Computer about, you know, certain topics.\n");
        
	return choose(
                44, "You go looking for a computer terminal", 
                55, "You go to the graduation ceremony immediately");
}

int page44()
{
	print_text("You walk down to a semi-secluded part of the training course complex and ");
	print_text("activate a computer terminal. \"AT YOUR SERVICE\" reads the computer screen.\n");

	if(read_letter == 0) return choose(23, "You register yourself as a mutant", 55, "You change your mind and go to the graduation ceremony");
        
        return choose3(
                23, "You register yourself as a mutant.", 
                46, "You want to chat about the commies.", 
                55, "You change your mind and go to the\n     graduation ceremony.");
        
}

int page45()
{
	print_text("\"Hrank Hrank,\" snorts the alarm in your living quarters.\n\nSomething is up.\n\n");
	print_text("You look at the monitor above the bathroom mirror and see the message you have ");
	print_text("been waiting for all these years.\n\n \"ATTENTION TROUBLESHOOTER, YOU ARE BEING ");
	print_text("ACTIVATED. PLEASE REPORT IMMEDIATELY TO MISSION ASSIGNMENT ROOM A17/GAMMA/LB22. ");
	print_text("THANK YOU. THE COMPUTER IS YOUR FRIEND.\"\n\nWhen you arrive at mission ");
	print_text("assignment room A17-gamma/LB22 you are given your previous clone\'s ");
	print_text("remaining possessions and notebook. You puzzle through your predecessor\'s ");
	print_text("cryptic notes, managing to decipher enough to lead you to the tube station and ");
	print_text("the tube car to GDH7-beta.");
        more();
        
	return 10;
}

int page46()
{
	print_text("\"Why do you ask about the communists, Troubleshooter?  It is not in the ");
	print_text("interest of your continued survival to be asking about such topics,\" says ");
	print_text("The Computer.\n");
        
	return choose(
                53, "You insist on talking about the communists", 
                54, "You change the subject");
}

int page47()
{
	print_text("The Computer orders the entire Vulture squadron to terminate the Troubleshooter ");
	print_text("Training Course.  Unfortunately you too are terminated for possessing ");
	print_text("classified information.\n\n");
	print_text("Don\'t act so innocent, we both know that you are an Illuminatus which is in ");
	print_text("itself an act of treason.\n\n");
	print_text("Don\'t look to me for sympathy.\n\n");
	print_text("			THE END\n");
        
	return 0;
}

int page48()
{
	print_text("The tubecar shoots forward as you enter, slamming you back into a pile of ");
	print_text("garbage. \n\nThe front end rotates upward and you, the garbage and the garbage ");
	print_text("disposal car shoot straight up out of Alpha Complex. \n\nOne of the last things ");
	print_text("you see is a small blue sphere slowly dwindling behind you.  After you fail to ");
	print_text("report in, you will be assumed dead.\n");
        more();
        
	return new_clone(45);
}

int page49()
{
	print_text("The instructor drags your inert body into a specimen detainment cage.\n\n");
	print_text("\"He\'ll make a good subject for tomorrow\'s mutant dissection class,\" you hear.\n");
        more();
        
	return new_clone(32);
}

int page50()
{
	print_text("You put down the other Troubleshooter, and then wisely decide to drill a few ");
	print_text("holes in the instructor as well; the only good witness is a dead witness.\n\n");
	print_text("You continue with the training course.\n");
        more();
        
	plato_clone++;
        
	return 41;
}

int page51()
{
	print_text("You run for it, but you don\'t run far. Three hundred strange and exotic ");
	print_text("weapons turn you into a freeze dried cloud of soot.\n");
        more();
        
	return new_clone(32);
}

int page52()
{
	print_text("You wisely wait until the instructor returns with a Blue Internal Security ");
	print_text("guard.\n\nThe guard leads you to an Internal Security self incrimination station.\n");
        more();
        
	return 2;
}

int page53()
{
	print_text("You tell The Computer about:\n");
        
	return choose(
                47, "The commies who have infiltrated the\n     Troubleshooter Training Course and\n     the impending People\'s Revolution", 
                54, "Something less dangerous");
}

int page54()
{
	print_text("\"Do not try to change the subject, Troubleshooter,\" says The Computer. ");
	print_text("\"It is a serious crime to ask about the communists. You will be terminated ");
	print_text("immediately.  Thank you for your inquiry.  The Computer is your friend.\"\n\n");
	print_text("Steel bars drop to your left and right, trapping you here in the hallway. ");
	print_text("A spotlight beams from the computer console to brilliantly iiluminate you while ");
	print_text("the speaker above your head rapidly repeats \"Traitor, Traitor, Traitor.\"\n\n");
	print_text("It doesn\'t take long for a few guards to notice your predicament and come to ");
	print_text("finish you off.\n");
        more();
        
	if (blast_door==0) return new_clone(45);
	else		   return new_clone(32);
}

int page55()
{
    char clone_text[256];
    
    intro = 1;
    clear();
    
    print_text("You and 300 other excited graduates are marched from the lecture hall and into ");
    print_text("a large auditorium for the graduation exercise. \n\nThe auditorium is ");
    print_text("extravagantly decorated in the colours of the graduating class. Great red and ");
    print_text("green plasti-paper ribbons drape from the walls, while a huge sign reading\n\n");
    print_text("\"Congratulations class of GDH7-beta-203.44/A\" hangs from the raised stage down ");
    print_text("front. \n\nOnce everyone finds a seat the ceremony begins. Jung-I-PSY is the ");
    print_text("first to speak, \"Congratulations students, you have successfully survived the ");
    print_text("Troubleshooter Training Course.\n\n");
    more();
    print_text("\"It always brings me great pride to address ");
    print_text("the graduating class, for I know, as I am sure you do too, that you are now ");
    print_text("qualified for the most perilous missions The Computer may select for you. The ");
    print_text("thanks is not owed to us of the teaching staff, but to all of you, who have ");
    print_text("persevered and graduated.  Good luck and die trying.\"\n\n");
    print_text("Then the instructor begins reading the names of the students who one by one walk to the front of ");
    print_text("the auditorium and receive their diplomas. Soon it is your turn, ");
    print_text("\"Philo-R-DMD, graduating a master of mutant identification and secret society ");
    sprintf(clone_text, "infiltration.\" You walk up and receive your diploma from Plato-B-PHI%d, then ", plato_clone);
    print_text("return to your seat. \n\n");
    more();
    print_text("There is another speech after the diplomas are handed ");
    print_text("out, but it is cut short by by rapid fire laser bursts from the high spirited ");
    print_text("graduating class. You are free to return to your barracks to wait, trained ");
    print_text("and fully qualified, for your next mission. You also get that cherished ");
    print_text("promotion from the Illuminati secret society. \n\nIn a week you receive a ");
    print_text("detailed Training Course bill totaling 1,523 credits.\n\n");
    print_text("			THE END\n");

    return 0;
}

int page56()
{
	print_text("That familiar strange feeling of deja\'vu envelops you again. \n\nIt is hard to ");
	print_text("say, but whatever is on the other side of the door does not seem to be intended ");
	print_text("for you.\n");
        
	return choose(
                33, "You open the door and step through", 
                22, "You go looking for more information");
}

int page57()
{
	print_text("In the center of the room is a table and a single chair. \n\nThere is an Orange ");
	print_text("folder on the table top, but you can\'t make out the lettering on it.\n");
        
	return choose(
                11, "You sit down and read the folder", 
                12, "You leave the room");
}

int next_page(int this_page)
{
    switch (this_page)
    {
        case  0 : return 0;
        case  1 : return page1();
        case  2 : return page2();
        case  3 : return page3();
        case  4 : return page4();
        case  5 : return page5();
        case  6 : return page6();
        case  7 : return page7();
        case  8 : return page8();
        case  9 : return page9();
        case 10 : return page10();
        case 11 : return page11();
        case 12 : return page12();
        case 13 : return page13();
        case 14 : return page14();
        case 15 : return page15();
        case 16 : return page16();
        case 17 : return page17();
        case 18 : return page18();
        case 19 : return page19();
        case 20 : return page20();
        case 21 : return page21();
        case 22 : return page22();
        case 23 : return page23();
        case 24 : return page24();
        case 25 : return page25();
        case 26 : return page26();
        case 27 : return page27();
        case 28 : return page28();
        case 29 : return page29();
        case 30 : return page30();
        case 31 : return page31();
        case 32 : return page32();
        case 33 : return page33();
        case 34 : return page34();
        case 35 : return page35();
        case 36 : return page36();
        case 37 : return page37();
        case 38 : return page38();
        case 39 : return page39();
        case 40 : return page40();
        case 41 : return page41();
        case 42 : return page42();
        case 43 : return page43();
        case 44 : return page44();
        case 45 : return page45();
        case 46 : return page46();
        case 47 : return page47();
        case 48 : return page48();
        case 49 : return page49();
        case 50 : return page50();
        case 51 : return page51();
        case 52 : return page52();
        case 53 : return page53();
        case 54 : return page54();
        case 55 : return page55();
        case 56 : return page56();
        case 57 : return page57();
        default : break;
    }
}

void main(int argc, char** argv)
{
    gamePathInit(argv[0]);
    
     // initialize SDL
    SDL_Init(SDL_INIT_VIDEO);
    SDL_WM_SetCaption("Paranoia", "Paranoia");
    renderer_init(&gamePath);
    
    srand(time(NULL));
    clear();
    instructions();
    more();
    character();
    more();
    
    while((page = next_page(page)) !=0) clear();
    
    print_options("Press SELECT to exit", " ");
    
    while(1) get_char(); // Spin lock until they exit
}
