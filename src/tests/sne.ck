// Managers ========================================================
InputManager IM;
spork ~ IM.start(0);

MouseManager MM;
spork ~ MM.start(0);

// Globals ==============================================
class Globals {
	// static vars
	0 => static int frameCounter;
	now => static time lastTime;
	0::samp => static dur deltaTime;

	// inputs
	-1 => static int snakeInput;
	Segment @ food;
	Grid @ grid;

} Globals G;

@(0.0, 0.0, 0.0) => vec3 ORIGIN;
@(0.0, 1.0, 0.0) => vec3 UP;
@(0.0, -1.0, 0.0) => vec3 DOWN;
@(1.0, 0.0, 0.0) => vec3 RIGHT;
@(-1.0, 0.0, 0.0) => vec3 LEFT;
@(0.0, 0.0, -1.0) => vec3 FORWARD;  // openGL uses right-handed sytem
@(0.0, 0.0, 1.0) => vec3 BACK;

fun string VecToString(vec3 v) {
	return v.x + "," + v.y + "," + v.z;
}

fun int VecEquals(vec3 a, vec3 b) {
	b - a => vec3 c;
	return c.magnitude() < .01;
}


// Resource Initialization ==================================================

// initialize geos
SphereGeometry  SphereGeometry ;
BoxGeometry boxGeo;
// init materials
NormalsMaterial normMat;  
NormalsMaterial headNormalsMaterial;
normMat.localNormals();  // use local space normals (so we can tell orientation)
headNormalsMaterial.polygonMode(Material.POLYGON_LINE);  // wireframe mode
// headNormalsMaterial.wireframe(true);

NormalsMaterial wireframeNormalsMaterial;
wireframeNormalsMaterial.polygonMode(Material.POLYGON_LINE);
// wireframeNormalsMaterial.wireframe(true);

// ECS classes ==================================================

class GameObject {
    fun void addComponent() {}
}

class Component {

}

// linked list for snake body
class Segment {

	null @=> Segment @ next;

	GMesh mesh;

	fun void Constructor(Geometry @ geo, Material @ mat) {
		// create mesh
		mesh.set(geo, mat);
	}

	fun int PartOfSnake() {
		return next != null;
	}

	fun static Segment @ Create(
		Geometry @ geo, 
		Material @ mat,
		vec3 pos
	) {
		Segment seg;
		seg.Constructor(geo, mat);
		seg.SetPos(pos);
		return seg;
	}

	fun Segment @ GetNext() {
		return this.next;
	}

	fun void SetNext(Segment @ seg) {
		seg @=> this.next;
	}

	fun vec3 GetPos() {
		return mesh.pos();
	}

	fun void SetPos(vec3 pos) {
		<<< "setpos", pos >>>;
		mesh.position(pos);
	}

	// returns whether point inside segment
	fun int Inside(vec3 point) {
		GetPos() => vec3 pos;
		<<< "is ", point, "inside seg at", pos >>>;
		return (
			Math.fabs(point.x - pos.x) <= .5
			&& Math.fabs(point.y - pos.y) <= .5
			&& Math.fabs(point.z - pos.z) <= .5
		);
		// (pos-point) => vec3 diff;
		// if (diff.magnitude() < .1)
		// {
		// 	<<< "yes!" >>>;
		// 	return true;
		// }
		// <<< "no!" >>>;
		// return false;
	} 
}

// class Laser {

// 	// Geometry
// 	GMesh laserMesh;
// 	BoxGeometry laserGeo;





class Grid {
    10 => int h;
    10 => int w;
    10 => int d;

    int minX, minY, minZ, maxX, maxY, maxZ;

	GMesh gridMesh;
	BoxGeometry gridGeo;

	NormalsMaterial gridMat;
	gridMat.polygonMode(Material.POLYGON_LINE);
	// gridMat.wireframe(true);

	// hashset of each grid cell
	vec3 emptyCells[];

// int erase( string key );
//     (map only) Erase all elements with the specified key.
// int find( string key );
//     (map only) Get number of elements with the specified key.
// void getKeys( string[] keys );
//     Return all keys found in associative array in keys

	fun void SetCellEmpty(vec3 pos) {
		pos => emptyCells[VecToString(pos)];
	}

	fun void SetCellFull(vec3 pos) {
		emptyCells.erase(VecToString(pos));
	}

	fun int IsCellEmpty(vec3 pos) {
		return emptyCells.isInMap(VecToString(pos));
	}

	fun vec3 GetRandomEmptyCell() {
		string keys[0];
		emptyCells.getKeys(keys);
		Math.random2(0, keys.size()-1) => int randIndex;
		return emptyCells[keys[randIndex]];
	}
	
    fun void Constructor(
		int height, int width, int depth,
		GScene@ scene
	) {
		height => this.h;
		width => this.w;
		depth => this.d;

		-w / 2 => minX; w / 2 => maxX;
		-h / 2 => minY; h / 2 => maxY;
		-d / 2 => minZ; d / 2 => maxZ;

		gridGeo.set(
			height * 1.0, width * 1.0, depth * 1.0,
			height, width, depth 
		);
		gridMesh.set(gridGeo, gridMat);
		gridMesh --> scene;

		// populate empty cells
		new vec3[0] @=> emptyCells;
		for (minX => int i; i <= maxX; i++) {
			for (minY => int j; j <= maxY; j++) {
				for (minZ => int k; k <= maxZ; k++) {
					@(i, j, k) => vec3 pos;
					SetCellEmpty(pos);
				}
			}
		}
    }

	// returns whether point is inside grid
    fun int Contains(vec3 pos) {
		return (
			pos.x >= minX && pos.x <= maxX &&
			pos.y >= minY && pos.y <= maxY &&
			pos.z >= minZ && pos.z <= maxZ
		);
    }
}


class Snake {
	GScene @ scene;
    Geometry @ geo;  // snake segment shape 
    Material @ mat;  // snake material

    GGen snakeObj;  // parent group for all segments
    // Segment @ segments[];  // list of individual segments. Tail is first element, head is last

	null @=> Segment @ head;
	null @=> Segment @ tail;

    // movement (relative)
	0 => static int SNAKE_UP;
	1 => static int SNAKE_DOWN;
	2 => static int SNAKE_LEFT;
	3 => static int SNAKE_RIGHT;

	FORWARD => vec3 curDir;  // movement direction in world space

    // TODO: quaternion slerping for auto camera pivot

	fun void SetHead(Segment @ seg) {
		if (this.head != null)
			this.head.mesh.set(geo, mat);  // reset head material

		seg @=> this.head;
		// seg.mesh.set(geo, headNormalsMaterial);  // set new head for debugging
		seg.mesh.set(geo, mat);  // set new head for debugging
	}

	// fun void SetTail(Segment @ seg) {
	// 	<<< "setting tail!" >>>;
	// 	// if (this.tail != null)
	// 	// 	this.tail.mesh.set(geo, mat);  // reset head material
	// 	seg @=> this.tail;
	// 	this.head.SetNext(this.tail);

	// 	// seg.mesh.set(SphereGeometry , mat);
	// }

    // constructor. call this first!
    fun void Constructor(
		Geometry @ snakeGeo, Material @ snakeMat,
		GScene @ scene
	) {
        snakeGeo @=> this.geo;
        snakeMat @=> this.mat;
		scene @=> this.scene;

		// add to scene
		snakeObj --> scene;

		// create 1-seg sneck
		Segment seg;
		seg.Constructor(this.geo, this.mat);

		SetHead(seg); seg @=> this.tail;
		tail.SetNext(head);
		head.SetNext(tail);

		// add to scene
		seg.mesh --> snakeObj;

		// spawn food
		SpawnFood();
    }

    // add new segment to head at position pos
    fun void AddSegment(vec3 pos) {
		Segment seg;
		seg.Constructor(this.geo, this.mat);

        // update position
        seg.SetPos(pos);

		// update grid state
		G.grid.SetCellFull(pos);

		AddSegment(seg);
    }

	// adds segment to head of snake
	fun void AddSegment(Segment @ seg) {
		// add to scene
		seg.mesh --> snakeObj;

		// update linked list
		seg.SetNext(tail);
		head.SetNext(seg);
		
		// update head
		SetHead(seg);
	}

	// spawn a food!
	fun void SpawnFood() {
		G.grid.GetRandomEmptyCell() => vec3 pos;
		Segment.Create(SphereGeometry , normMat, pos) @=> Segment @ food;

		// update state
		G.grid.SetCellFull(pos);
		food @=> G.food;

		food.mesh --> scene;
	}

    // moves the snake by 1 step
    fun void Slither() {
		<<< "Slither" >>>;
		if (this.head == this.tail) {
			<<< "head === tail" >>>;
		}
		// rather than move position of every segment, we only need to change
		// the tail and move it to the new head

		curDir + head.GetPos() => vec3 newPos;
		tail.GetPos() => vec3 oldPos;


		// slither onwards if no collisions
		if (G.grid.IsCellEmpty(newPos)) {

			// set tail to new pos
			tail.SetPos(newPos);

			// update new tail, head, and pointers
			SetHead(this.head.GetNext());
			this.tail.GetNext() @=> this.tail;

			// update emptycell set
			G.grid.SetCellEmpty(oldPos);
			G.grid.SetCellFull(newPos);
			return;
		} 

		// collisions!

		// case 1: collision with wall
		if (!G.grid.Contains(newPos)) {
			<<< "collision with wall" >>>;
			return;
		}

		// case 2: collision with food
		else if (
			G.food != null &&
			VecEquals(newPos, G.food.GetPos())
		) {
			<<< "collision with food" >>>;
			// add new segment to head
			AddSegment(G.food);
			null @=> G.food;

			G.grid.SetCellFull(newPos);

			// spawn new food
			SpawnFood();
			return;
		} 
		
		// case 3: collision with self
		else {
			<<< "collision with self" >>>;
		}
    }

	// change snake direction according to input
	// wonder if there's a cleaner way to do this
	fun void UpdateDir(int input) {
		if (input < 0 ) return;  // base case, no input. continue along current direction
		if (curDir == FORWARD) {
			if (input == SNAKE_UP) { UP => curDir; }
			else if (input == SNAKE_DOWN) { DOWN => curDir; }
			else if (input == SNAKE_LEFT) { LEFT => curDir; }
			else if (input == SNAKE_RIGHT) { RIGHT => curDir; }
		} else if (curDir == BACK) {
			if (input == SNAKE_UP) { UP => curDir; }
			else if (input == SNAKE_DOWN) { DOWN => curDir; }
			else if (input == SNAKE_LEFT) { RIGHT => curDir; }
			else if (input == SNAKE_RIGHT) { LEFT => curDir; }
		} else if (curDir == RIGHT) {
			if (input == SNAKE_UP) { UP => curDir; }
			else if (input == SNAKE_DOWN) { DOWN => curDir; }
			else if (input == SNAKE_LEFT) { FORWARD => curDir; }
			else if (input == SNAKE_RIGHT) { BACK => curDir; }
		} else if (curDir == LEFT) {
			if (input == SNAKE_UP) { UP => curDir; }
			else if (input == SNAKE_DOWN) { DOWN => curDir; }
			else if (input == SNAKE_LEFT) { BACK => curDir; }
			else if (input == SNAKE_RIGHT) { FORWARD => curDir; }
		} else if (curDir == UP) {
			if (input == SNAKE_UP) { BACK => curDir; }
			else if (input == SNAKE_DOWN) { FORWARD => curDir; }
			else if (input == SNAKE_LEFT) { LEFT => curDir; }
			else if (input == SNAKE_RIGHT) { RIGHT => curDir; }
		} else if (curDir == DOWN) {
			if (input == SNAKE_UP) { FORWARD => curDir; }
			else if (input == SNAKE_DOWN) { BACK => curDir; }
			else if (input == SNAKE_LEFT) { LEFT => curDir; }
			else if (input == SNAKE_RIGHT) { RIGHT => curDir; }
		}
	}


}


// Scene Setup =============================================================
NextFrameEvent UpdateEvent;

GCamera mainCamera; 
GScene scene;

Grid grid;
grid.Constructor(21, 21, 21, scene);
grid @=> G.grid;

Snake snake;
snake.Constructor(boxGeo, normMat, scene);





fun void Movement() {
	while (.5::second => now) {
		snake.UpdateDir(G.snakeInput);
		if (IM.isKeyDown(IM.KEY_SPACE))
			snake.AddSegment(snake.head.GetPos() + snake.curDir);
		else
			snake.Slither();
		-1 => G.snakeInput; // reset input state
	}
}
spork ~ Movement();

fun void SnakeInputHandler() {
	while (true) {
		IM.anyKeyDownEvent => now;
		if (IM.isKeyDown(IM.KEY_UP)) { Snake.SNAKE_UP => G.snakeInput; }
		else if (IM.isKeyDown(IM.KEY_DOWN)) { Snake.SNAKE_DOWN => G.snakeInput; }
		else if (IM.isKeyDown(IM.KEY_LEFT)) { Snake.SNAKE_LEFT => G.snakeInput; }
		else if (IM.isKeyDown(IM.KEY_RIGHT)) { Snake.SNAKE_RIGHT => G.snakeInput; }
	}
}

spork ~ SnakeInputHandler();

// barf...
// fun void WorldMovement() {
// 	while (true) {
// 		10::ms => now;
// 		scene.rotateOnLocalAxis(UP, 0.002);
// 	}
// } spork ~ WorldMovement();




fun void Update() {

}


// flycamera controls
fun void cameraUpdate(time t, dur dt)
{

	// set camera to always be above snake head
	// TODO: needs to be relative to the snake direction
	// OR just have wasd control camera pivot
	// OR use orbit camera controls
	// mainCamera.position(snake.head.GetPos() + @(0.0, 3.0, 3.0));
	// OR make grid, snake, everything child of one group
	// and use mouse/keys to ROTATE the group itself, like spinning the whole world around

	// mouse lookaround
	.001 => float mouseSpeed;
	MM.GetDeltas() * mouseSpeed => vec3 mouseDeltas;

	// for mouse deltaY, rotate around right axis
	mainCamera.rotateOnLocalAxis(RIGHT, -mouseDeltas.y);

	// for mouse deltaX, rotate around (0,1,0)
	mainCamera.rotateOnWorldAxis(UP, -mouseDeltas.x);

	2.5 * (dt / second) => float cameraSpeed;
	if (IM.isKeyDown(IM.KEY_LEFTSHIFT))
		2.5 *=> cameraSpeed;
	// camera movement
	if (IM.isKeyDown(IM.KEY_W))
		mainCamera.translate(cameraSpeed * mainCamera.forward());
	if (IM.isKeyDown(IM.KEY_S))
		mainCamera.translate(-cameraSpeed * mainCamera.forward());
	if (IM.isKeyDown(IM.KEY_D))
		mainCamera.translate(cameraSpeed * mainCamera.right());
	if (IM.isKeyDown(IM.KEY_A))
		mainCamera.translate(-cameraSpeed * mainCamera.right());
	if (IM.isKeyDown(IM.KEY_Q))
		mainCamera.translate(cameraSpeed * UP);
	if (IM.isKeyDown(IM.KEY_E))
		mainCamera.translate(-cameraSpeed * UP);

}


// Game loop 
fun void GameLoop(){
	// GG.Render(); // kick of the renderer
	while (true) {
		// UpdateEvent => now;
		// <<< "==== update loop " >>>;
		1 +=> G.frameCounter;
		
		// compute timing
		now - G.lastTime => G.deltaTime;
		now => G.lastTime;

		// Update logic
		cameraUpdate(now, G.deltaTime);
		Update();

		// End update, begin render
		// GG.Render();
		GG.nextFrame() => now;
	}
} 

GameLoop();
