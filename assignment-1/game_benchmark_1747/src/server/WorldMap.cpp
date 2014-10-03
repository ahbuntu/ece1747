#include "ServerData.h"
#include "WorldMap.h"

void WorldMap::generate()
{
	int i, j;
	Vector2D pos;

    /* generate terrain */
	terrain = new char*[size.x];
	for ( i = 0; i < size.x; i++ )
	{
		terrain[i] = new char[size.y];
		for ( j = 0; j < size.y; j++ )
		{
			terrain[i][j] = ( rand() % 1000 < blocks ) ? 1 : 0;
		}
	}

	players = new PlayerBucket[ sd->num_threads ];
	n_players = 0;

	list<Player*> pls;
	list<GameObject*> objs;
	
	n_regs.x = size.x / regmin.x;
	n_regs.y = size.y / regmin.y;
	int regions_per_thread = (n_regs.x * n_regs.y - 1) / sd->num_threads + 1;
	regions = new Region*[ n_regs.x ];
	for( i = 0, pos.x = 0; i < n_regs.x; i++, pos.x += regmin.x )
	{
		regions[i] = new Region[ n_regs.y ];
		for( j = 0, pos.y = 0; j < n_regs.y; j++, pos.y += regmin.y )
		{
			initRegion( &regions[i][j], pos, regmin, (i*n_regs.y + j)/regions_per_thread, objs, pls);
		}
	}

	/* generate objects */
	GameObject *o;
	Region* r;
			
	for ( i = 0; i < resources * size.x * size.y / 1000; i++ )
	{
		o = new GameObject();			
    while(1)
    {
			o->pos.x = rand() % size.x;
			o->pos.y = rand() % size.y;
			if( terrain[o->pos.x][o->pos.y] != 0  )
			{
				continue;
			}

			r = getRegionByLocation( o->pos );
			assert(r);

			if( Region_addObject( r, o, min_res, max_res ) )
			{
				break;
			}
		}
	}
}

Player* WorldMap::addPlayer( IPaddress a )
{
	/* create a new player */
	Player* p = new Player( a );
	assert( p );

	/* set player name and initial attributes */
	char pname[MAX_PLAYER_NAME];
	sprintf(pname, "Player_%X_%d", a.host, a.port);
	p->setName(pname);	
	p->life = sd->player_min_life + rand() % ( sd->player_max_life - sd->player_min_life + 1 );
	p->attr = sd->player_min_attr + rand() % ( sd->player_max_attr - sd->player_min_attr + 1 );
	p->time_of_last_message = SDL_GetTicks();
	
	Region* r = NULL;
	
	while(1)
	{
		p->pos.x = rand() % size.x;
		p->pos.y = rand() % size.y;
		if( terrain[p->pos.x][p->pos.y] != 0  )
		{
			continue;
		}
		
		r = getRegionByLocation( p->pos );
		assert(r);

		if( Region_addPlayer(r, p) )
		{
			break;
		}
	}

	players[ r->layout ].insert(p);

	return p;
}

Player* WorldMap::findPlayer( IPaddress a, int t_id )
{
	int i;
	Player* p = NULL;  
	for( i = 0; i < sd->num_threads; i++ )
	{
		p = players[ (t_id+i) % sd->num_threads ].find( a );		
		if( p )
		{
			break;
		}
	}
	return p;
}

void WorldMap::removePlayer(Player* p)
{
	Region* r = getRegionByLocation( p->pos );
	Region_removePlayer(r, p);
	players[ r->layout ].erase(p);
}

void WorldMap::movePlayer(Player* p)
{
	Vector2D n_pos = p->pos;
	if( p->dir == 0 )	n_pos.y = p->pos.y + 1;		// DOWN
	if( p->dir == 1 )	n_pos.x = p->pos.x + 1;		// RIGHT
	if( p->dir == 2 )	n_pos.y = p->pos.y - 1;		// UP
	if( p->dir == 3 )	n_pos.x = p->pos.x - 1;		// LEFT
	
	/* the player is on the edge of the map */
	if ( n_pos.x < 0 || n_pos.x >= size.x || n_pos.y < 0 || n_pos.y >= size.y )	
	{
		return;
	}

	/* client tries to move to a blocked area */
	if ( terrain[ n_pos.x ][ n_pos.y ] != 0 )
	{
		return;
	}

	/* get old and new region */
	Region *r_old = getRegionByLocation( p->pos );
	Region *r_new = getRegionByLocation( n_pos );
	assert( r_old && r_new );
	
	bool res = Region_movePlayer( r_old, r_new, p, n_pos );
	if( r_old->layout != r_new->layout && res )
	{
		players[ r_old->layout ].erase( p );
		players[ r_new->layout ].insert( p );
	}
}


void WorldMap::useGameObject(Player* p)
{
	assert(p);
	Region*		r = getRegionByLocation( p->pos );
	assert( r );	

	GameObject*	o = Region_getObject( r, p->pos );
	
	// Fix for UDP out of order bug
	if ( o )
	{
		if ( o->quantity > 0 )
		{
			p->useObject(o);
		}
	}
}

void WorldMap::attackPlayer(Player* p, int attack_dir)
{
	assert(p);
	/* get second player */
	Vector2D pos2 = p->pos;
	if( attack_dir == 0 )	pos2.y = p->pos.y + 1;		// DOWN
	if( attack_dir == 1 )	pos2.x = p->pos.x + 1;		// RIGHT
	if( attack_dir == 2 )	pos2.y = p->pos.y - 1;		// UP
	if( attack_dir == 3 )	pos2.x = p->pos.x - 1;		// LEFT

	/* check if coordinates are inside the map  */
	if( pos2.x < 0 || pos2.x >= size.x || pos2.y < 0 || pos2.y >= size.y ) {
		return;
	}

	/* get second player */
	Region* r  = getRegionByLocation( pos2 ); 
	assert( r );

	Player* p2 = Region_getPlayer( r, pos2 );
	if ( p2 != NULL )
	{
		p->attackPlayer( p2 );
	}

	if ( sd->display_actions )
	{	
		printf("Player %s attacks %s\n", p->name, p2->name);
	}
}

void packRegion( Region* r, Serializator* s, Player* p, Vector2D pos1, Vector2D pos2 )
{
	Player* p2;
	GameObject* o; 
	list<Player*>::iterator ip;
    list<GameObject*>::iterator io;
    
    for( ip = r->players.begin(); ip != r->players.end(); ip++ )
    {
    	p2 = *ip;
    	if( p2 == p )	continue;
    	if( p2->pos.x < pos1.x || p2->pos.y < pos1.y || p2->pos.x >= pos2.x || p2->pos.y >= pos2.y )	continue;
    	
    	*s << CELL_PLAYER;
    	*s << p2->pos.x;
    	*s << p2->pos.y;
        *s << p2->life;
        *s << p2->attr;
        *s << p2->dir;
        /* IPaddress used as an ID: */
        s->putBytes((char*)&p2->address, sizeof(IPaddress));
    }
    
    for( io = r->objects.begin(); io != r->objects.end(); io++ )
    {
    	o = *io;
    	if( o->quantity <= 0 )	continue;
    	if( o->pos.x < pos1.x || o->pos.y < pos1.y || o->pos.x >= pos2.x || o->pos.y >= pos2.y )		continue;
    	
    	*s << CELL_OBJECT;
        *s << o->pos.x;
    	*s << o->pos.y;
        *s << o->attr;
        *s << o->quantity;
    }
}

void WorldMap::updatePlayer(Player* p, Serializator* s)
{
	int i,j;
    Vector2D pos1,pos2;				/* rectangular region visible to the player */
    Vector2D loc;
    
    /* determine the region visible to the player */
		pos1.x = max(p->pos.x - MAX_CLIENT_VIEW, 0);		pos1.y = max(p->pos.y - MAX_CLIENT_VIEW, 0);
		pos2.x = min(p->pos.x + MAX_CLIENT_VIEW+1, size.x);	pos2.y = min(p->pos.y + MAX_CLIENT_VIEW+1, size.y);    

    /* pack data: position, attributes */
    *s << p->pos.x;	*s << p->pos.y;
    *s << pos1.x;	*s << pos1.y;	*s << pos2.x;	*s << pos2.y;
    *s << p->life;	*s << p->attr;	*s << p->dir;

    /* pack data: terrain, objects, players */
    for( i = pos1.x; i < pos2.x; i++ )    	for( j = pos1.y; j < pos2.y; j++ )		*s << terrain[i][j];
    
    loc.x = pos1.x; loc.y = pos1.y;
	packRegion( getRegionByLocation( loc ), s, p, pos1, pos2 );
	
	if( (pos2.x-1)/regmin.x != pos1.x/regmin.x )
	{
		loc.x = pos2.x-1; loc.y = pos1.y;
		packRegion( getRegionByLocation( loc ), s, p, pos1, pos2 );
	}
	if( (pos2.y-1)/regmin.y != pos1.y/regmin.y )
	{
		loc.x = pos1.x; loc.y = pos2.y-1;
		packRegion( getRegionByLocation( loc ), s, p, pos1, pos2 );
	}
	if( (pos2.x-1)/regmin.x != pos1.x/regmin.x && (pos2.y-1)/regmin.y != pos1.y/regmin.y )
	{
		loc.x = pos2.x-1; loc.y = pos2.y-1;
		packRegion( getRegionByLocation( loc ), s, p, pos1, pos2 );
	}	
    
    *s << CELL_NONE;  
}

Region*	WorldMap::getRegionByLocation( Vector2D loc)
{
	return &regions[ loc.x/regmin.x ][ loc.y/regmin.y ];
}

void	WorldMap::regenerateObjects()
{
	Region_regenerateObjects( &regions[ rand() % n_regs.x ][ rand() % n_regs.y ], max_res );
}

void	WorldMap::rewardPlayers( Vector2D quest_pos )
{
	Region_rewardPlayers( &regions[ quest_pos.x/CLIENT_MATRIX_SIZE ][ quest_pos.y/CLIENT_MATRIX_SIZE ], sd->quest_bonus, sd->player_max_life );
}


void WorldMap::reassignRegion( Region* r, int new_layout )
{
	list<Player*>::iterator pi;			//iterator for players
	
	for ( pi = r->players.begin(); pi != r->players.end(); pi++ )
	{
		players[ r->layout  ].erase(  *pi );
		players[ new_layout ].insert( *pi );
	}
	r->layout = new_layout;
}


/*
Lightest is a dynamic load balancing algorithm that attempts to
optimize the cost of remapping by prioritizing shedding load to a
single server instead of several servers. The algorithm does not take
network proximity in the game (i.e., to neighbors) into account.
Furthermore, clustering of adjacent regions is maintained whenever
possible but is of secondary concern compared to load shedding to
a single server.

An overloaded server tries to shed load directly to the lightest
loaded node known. The precondition is that this node’s load has
to be below Light_load_th. Note that our definition of Light_load_th
is such that a sigle such node should be able to accommodate a
sufficient load shed from an overloaded node. While this is true in
most cases, depending on the actual distribution of players to regions,
if some regions are more overloaded than others, a careful
load shedding schedule should be constructed to maximize the load
to be shed. The lightest loaded node may in fact be a neighbor, but
the Lightest algorithm does not give preference to neighbors when
shedding load except in case of load ties. Instead, the overloaded
node prefers shedding a sufficient portion of its load to a single
server even if this server is remote and even if this implies some
declustering for the shedded regions. Regions of the overloaded
node are scanned in-order and placed into a bin to be assigned to
the lightly loaded node. Thus, Lightest attempts to keep region
clusters together by scanning adjacent regions in sequence. On the
other hand, if a region cannot be placed in the bin without exceeding
the safe load threshold, a subsequent region is selected, hence
sacrificing on region locality. In contrast, the main Locality Aware
algorithm prioritizes region locality.
*/
void WorldMap::balance_lightest()
{

	//Definitions:
	//Overload threshold = server/thread load in number of clients for which SLA is violated
	//SLA violation = server/thread exceeds predefined update interval for 90% of its clients
	//Safe load threshold = highest load (# of clients) for which the SLA is met for ALL clients
	//Light load threshold = 2*safety_th - overload_th
	//Thread in overload = #clients in thread > overload_th


	//To Do:
	// 1. compute overload threshold for each thread
	// 2. compute safe load threshold for each thread
	// 3. determine if thread is in overload (implement isOverloaded() defined in header)
	// 4. identify lightest node/thread (load < Light_load_th)	
	// 5. identify regions to be reassigned 
	// 6. reassign overloaded region(s) to lightest thread (reassignRegion() already implemented)
	
	
	//Example Scenario:
	//T1 = 20 clients
	//T2 = 50 clients
	//T3 = 30 clients
	//T4 = 10 clients
	//SLA = all clients receive updates in 50 ms
	//T1: 17 clients receive updates > 50 ms => 17/20 (85% clients have SLA violations)
	//T2: 47 clients receive updates > 50 ms => 47/50 (94% clients have SLA violations)
	//T3: 5 clients receive updates > 50 ms  => 5/30 (17% clients have SLA violations)
	//T4: 0 clients receive updates > 50 ms 
	
	//Open Questions for the previous example:
	//Q1 - how do we calculate the overlod threshold for each thread?
	//Q2 - which threads are in overload?
	//Q3 - how do we calculate the safe load threshold for each thread?
}

/* 
Spread is a dynamic load balancing algorithm that aims at optimizing
the overall load balancing through a uniform load spread in
the server network. Load shedding is triggered when the number
of players exceeds a single-server’s capacity (in terms of CPU or
bandwidth towards its clients) just as in our main dynamic partitioning
algorithm.
This algorithm, however, attempts to uniformly spread the players
across all participating servers through global reshuffling of regions
to servers. The algorithm is meant to be an extreme where
the primary concern is the global optimum in terms of the smallest
difference between the lowest and highest loaded server in the
system. There are no attempts at locality preservation in either
network proximity or region adjacency. The algorithm is a binpacking
algorithm that needs global load information for all participating
servers. The algorithm starts with one empty bin per server,
then in successive steps takes a region that is currently mapped at
any of the servers and places it in the bin with the current lightest
load. This is done iteratively until all regions have been placed
into bins. After the global reshuffle schedule is created, each bin is
assigned to a particular server and the corresponding region migrations
occur. While the algorithm could be adapted to include only
a subset of servers (e.g., just neighbors and their neighbors) into
the region reshuffle process, we currently involve all servers in this
process.
*/
void WorldMap::balance_spread()
{
}

void WorldMap::balance()
{
	Uint32 now = SDL_GetTicks();
	if ( now - last_balance < sd->load_balance_limit ) {
		return;
	}

	last_balance = now;
	
	if( !strcmp( sd->algorithm_name, "static" ) ) {
		return;
	}
	
	n_players = 0;
	for( int i = 0; i < sd->num_threads; i++ ) {
		n_players += players[i].size();
	}

	if( n_players == 0 ) {
		return;
	}
	
	if( !strcmp( sd->algorithm_name, "lightest" ) ) {
		return balance_lightest();
	} else if( !strcmp( sd->algorithm_name, "spread" ) ) {
		return balance_spread();
	}

	printf("Algorithm %s is not implemented.\n", sd->algorithm_name);
	return;
}
