#include <iostream>
#include <chrono>
#include <SDL2/SDL.h>
#include <fstream>
#include <random>
#include <cstring>

class Chip8 {
    public:
        uint8_t V[16]{};
        uint8_t memory[4096]{};
        uint16_t I{};
        uint16_t pc{};
        uint16_t stack[16]{};
        uint8_t sp{};
        uint8_t delay_timer{};
        uint8_t sound_timer{};
        uint8_t keypad[16]{};
        uint32_t gfx[64 * 32]{};
        uint16_t opcode;

        std::default_random_engine randGen;
	    std::uniform_int_distribution<uint8_t> randByte;

        /*memory map of system
        0x000-0x1FF - Chip 8 interpreter (contains font set in emu)
        0x050-0x0A0 - Used for the built in 4x5 pixel font set (0-F)
        0x200-0xFFF - Program ROM and work RAM

        */
    //constructer 
    
    const uint16_t START_ADDRESS = 0x200;
    const uint16_t FONTSET_START_ADDRESS = 0x50;
    uint16_t fontset[80] =
    {
        0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
        0x20, 0x60, 0x20, 0x20, 0x70, // 1
        0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
        0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
        0x90, 0x90, 0xF0, 0x10, 0x10, // 4
        0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
        0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
        0xF0, 0x10, 0x20, 0x40, 0x40, // 7
        0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
        0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
        0xF0, 0x90, 0xF0, 0x90, 0x90, // A
        0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
        0xF0, 0x80, 0x80, 0x80, 0xF0, // C
        0xE0, 0x90, 0x90, 0x90, 0xE0, // D
        0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
        0xF0, 0x80, 0xF0, 0x80, 0x80  // F
    };
    Chip8() 
    : randGen(std::chrono::system_clock::now().time_since_epoch().count()) {
        pc = START_ADDRESS;

        //load font
        for (int i = 0; i < 80; ++i) {
            memory[FONTSET_START_ADDRESS + i] = fontset[i];
        }
        table[0x0] = &Chip8::Table0;
		table[0x1] = &Chip8::OP_1NNN;
		table[0x2] = &Chip8::OP_2NNN;
		table[0x3] = &Chip8::OP_3XNN;
		table[0x4] = &Chip8::OP_4XNN;
		table[0x5] = &Chip8::OP_5XY0;
		table[0x6] = &Chip8::OP_6XNN;
		table[0x7] = &Chip8::OP_7XNN;
		table[0x8] = &Chip8::Table8;
		table[0x9] = &Chip8::OP_9XY0;
		table[0xA] = &Chip8::OP_ANNN;
		table[0xB] = &Chip8::OP_BNNN;
		table[0xC] = &Chip8::OP_CXNN;
		table[0xD] = &Chip8::OP_DXYN;
		table[0xE] = &Chip8::TableE;
		table[0xF] = &Chip8::TableF;

		for (size_t i = 0; i <= 0xE; i++)
		{
			table0[i] = &Chip8::OP_NULL;
			table8[i] = &Chip8::OP_NULL;
			tableE[i] = &Chip8::OP_NULL;
		}

		table0[0x0] = &Chip8::OP_00E0;
		table0[0xE] = &Chip8::OP_00EE;

		table8[0x0] = &Chip8::OP_8XY0;
		table8[0x1] = &Chip8::OP_8XY1;
		table8[0x2] = &Chip8::OP_8XY2;
		table8[0x3] = &Chip8::OP_8XY3;
		table8[0x4] = &Chip8::OP_8XY4;
		table8[0x5] = &Chip8::OP_8XY5;
		table8[0x6] = &Chip8::OP_8XY6;
		table8[0x7] = &Chip8::OP_8XY7;
		table8[0xE] = &Chip8::OP_8XYE;

		tableE[0x1] = &Chip8::OP_EXA1;
		tableE[0xE] = &Chip8::OP_EX9E;

		for (size_t i = 0; i <= 0x65; i++)
		{
			tableF[i] = &Chip8::OP_NULL;
		}

		tableF[0x07] = &Chip8::OP_FX07;
		tableF[0x0A] = &Chip8::OP_FX0A;
		tableF[0x15] = &Chip8::OP_FX15;
		tableF[0x18] = &Chip8::OP_FX18;
		tableF[0x1E] = &Chip8::OP_FX1E;
		tableF[0x29] = &Chip8::OP_FX29;
		tableF[0x33] = &Chip8::OP_FX33;
		tableF[0x55] = &Chip8::OP_FX55;
		tableF[0x65] = &Chip8::OP_FX65;

    }
    void Table0()
	{
		((*this).*(table0[opcode & 0x000Fu]))();
	}

	void Table8()
	{
		((*this).*(table8[opcode & 0x000Fu]))();
	}

	void TableE()
	{
		((*this).*(tableE[opcode & 0x000Fu]))();
	}

	void TableF()
	{
		((*this).*(tableF[opcode & 0x00FFu]))();
	}

	void OP_NULL()
	{}

    void LoadROM(char const* filename) {
        // Open the file as a stream of binary and move the file pointer to the end
        std::ifstream file(filename, std::ios::binary | std::ios::ate);

        if (file.is_open())
        {
            // Get size of file and allocate a buffer to hold the contents
            std::streampos size = file.tellg();
            char* buffer = new char[size];

            // Go back to the beginning of the file and fill the buffer
            file.seekg(0, std::ios::beg);
            file.read(buffer, size);
            file.close();

            // Load the ROM contents into the Chip8's memory, starting at 0x200
            for (long i = 0; i < size; ++i)
            {
                memory[START_ADDRESS + i] = buffer[i];
            }

            // Free the buffer
            delete[] buffer;
        }
    }
    void OP_00E0 () {
        //clear screen
        memset(gfx, 0, sizeof(gfx));
    }
    void OP_00EE(){
        //return from subroutine
        --sp;
        pc = stack[sp];
    }
    void OP_1NNN() {
        //jump to memory address NNN
        uint16_t address = opcode & 0x0FFFu;
        pc = address;
    }
    void OP_2NNN() {
        //call subroutine at NNN
        uint16_t address = opcode & 0x0FFFu;
        stack[sp] = pc;
        sp++;
        pc = address;
    }
    void OP_3XNN() {
        //Skip the following instruction if the value of register VX equals NN 
	    uint16_t x = (opcode & 0x0F00u) >> 8u;    
        uint16_t num = opcode & 0x00FFu;
        if (V[x] == num) {
            pc += 2;
        }
    }
    void OP_4XNN() {
        //Skip the following instruction if the value of register VX doesn't equal NN 
	    uint16_t x = (opcode & 0x0F00u) >> 8u;    
        uint16_t num = opcode & 0x00FFu;
        if (V[x] != num) {
            pc += 2;
        }
    }
    void OP_5XY0() {
        //Skip if VX == VY
        uint16_t x = (opcode & 0x0F00u) >> 8u;
        uint16_t y = (opcode & 0x00F0u) >> 4u;

        if (V[x] == V[y]) {
            pc += 2;
        }
    }
    void OP_6XNN() {
        //Store NN in register X
	    uint16_t x = (opcode & 0x0F00u) >> 8u;    
        uint16_t num = opcode & 0x00FFu;
        V[x] = num;
    }
    void OP_7XNN() {
        //Add NN to register X
	    uint16_t x = (opcode & 0x0F00u) >> 8u;    
        uint16_t num = opcode & 0x00FFu;
        V[x] += num;
    }
    void OP_8XY0() {
        //store y in x
        uint16_t x = (opcode & 0x0F00u) >> 8u;
        uint16_t y = (opcode & 0x00F0u) >> 4u;

        V[x] = V[y];
    }
    void OP_8XY1() {
        //x is x OR y
        uint16_t x = (opcode & 0x0F00u) >> 8u;
        uint16_t y = (opcode & 0x00F0u) >> 4u;

        V[x] =(V[x] | V[y]);
    }
    void OP_8XY2() {
        //x is x AND y
        uint16_t x = (opcode & 0x0F00u) >> 8u;
        uint16_t y = (opcode & 0x00F0u) >> 4u;

        V[x] =(V[x] & V[y]);
    }
    void OP_8XY3() {
        //x is x XOR y
        uint16_t x = (opcode & 0x0F00u) >> 8u;
        uint16_t y = (opcode & 0x00F0u) >> 4u;

        V[x] =(V[x] ^ V[y]);
    }
    void OP_8XY4() {
        //Add the value of register VY to register VX, Set VF to 01 if a carry occurs, Set VF to 00 if a carry does not occur
        uint16_t x = (opcode & 0x0F00u) >> 8u;
        uint16_t y = (opcode & 0x00F0u) >> 4u;

        uint16_t sum = V[x] + V[y];
        if (sum > 255u) {
            V[0xF] = 1;
        } else {
            V[0xF] = 0;
        }
        V[x] = sum & 0xFFu;
    }
    void OP_8XY5() {
        //Subtract the value of register VY to register VX, Set VF to 00 if a borrow occurs, Set VF to 01 if a borrow does not occur
        uint16_t x = (opcode & 0x0F00u) >> 8u;
        uint16_t y = (opcode & 0x00F0u) >> 4u;

        if (V[x] > V[y]) {
            V[0xF] = 1;
        } else {
            V[0xF] = 0;
        }
        V[x] -= V[y];
    }
    void OP_8XY6() {
        //divide by 2, store evenness in VF
        uint16_t x = (opcode & 0x0F00u) >> 8u;
        uint16_t y = (opcode & 0x00F0u) >> 4u;

        V[x] = V[y] / 2;
        if (V[y] % 2 == 0) {
            V[0xF] = 1;
        } else {
            V[0xF] = 0;
        }
    }
    void OP_8XY7() {
        //VY - VX Set VF to 00 if a borrow occurs, Set VF to 01 if a borrow does not occur
        uint16_t x = (opcode & 0x0F00u) >> 8u;
        uint16_t y = (opcode & 0x00F0u) >> 4u;

        if (V[x] > V[y]) {
            V[0xF] = 1;
        } else {
            V[0xF] = 0;
        }
        V[x] = V[y] - V[x];
    }
    void OP_8XYE() {
    //left shift VY and store in VX, then store MSB in VF
        uint16_t x = (opcode & 0x0F00u) >> 8u;
        uint16_t y = (opcode & 0x00F0u) >> 4u;

        // Save MSB in VF
        V[0xF] = (V[y] & 0x80u) >> 7u;

        V[x] = V[y] << 1;
    }
    void OP_9XY0() {
        //Skip if VX != VY
        uint16_t x = (opcode & 0x0F00u) >> 8u;
        uint16_t y = (opcode & 0x00F0u) >> 4u;

        if (V[x] != V[y]) {
            pc += 2;
        }
    }
    void OP_ANNN() {
        //store NNN in I
        uint16_t address = opcode & 0x0FFFu;
        I = address;

    }
    void OP_BNNN() {
        //jump to NNN + V0
        uint16_t address = opcode & 0x0FFFu;
        pc = address + V[0];
    }
    void OP_CXNN() {
        //set VX to a random number with mask NN
        uint16_t x = (opcode & 0x0F00u) >> 8u;    
        uint16_t mask = opcode & 0x00FFu;
        V[x] = randByte(randGen) & mask;
    }
    void OP_DXYN() {
	uint16_t x = (opcode & 0x0F00u) >> 8u;
	uint16_t y = (opcode & 0x00F0u) >> 4u;
	uint16_t height = opcode & 0x000Fu;

	// Wrap if going beyond screen boundaries
	uint16_t xPos = V[x] % 64;
	uint16_t yPos = V[y] % 32;

	V[0xF] = 0;

        for (uint16_t row = 0; row < height; ++row) {
            uint8_t spriteByte = memory[I + row];

            for (uint16_t col = 0; col < 8; ++col) {
                uint16_t spritePixel = spriteByte & (0x80u >> col);
                uint32_t * screenPixel = &gfx[(yPos + row) * 64 + (xPos + col)];

                // Sprite pixel is on
                if (spritePixel) {
                    // Screen pixel also on - collision
                    if (*screenPixel == 0xFFFFFFFF) {
                        V[0xF] = 1;
                    }

                    // Effectively XOR with the sprite pixel
                    *screenPixel ^= 0xFFFFFFFF;
                }
            }
        }
    }

    void OP_EX9E (){
        //skip if key with value vX is pressed
        uint16_t x = (opcode & 0x0F00u) >> 8u;
        uint16_t key = V[x];
        if (keypad[key]){
            pc += 2;
        }
    }
    void OP_EXA1 () {
        //skip if not pressed
        uint16_t x = (opcode & 0x0F00u) >> 8u;
        uint16_t key = V[x];
        if (!keypad[key]){
            pc += 2;
        }
    }
    void OP_FX07 () {
        //store the current value of the delay timer in vX
        uint16_t x = (opcode & 0x0F00u) >> 8u;
        V[x] = delay_timer;
        
    }
    void OP_FX0A () {
        //wait for keypress and store value in vX
        uint16_t x = (opcode & 0x0F00u) >> 8u;
        	if (keypad[0]) {
                V[x] = 0;
            }
            else if (keypad[1]) {
                V[x] = 1;
            }
            else if (keypad[2]) {
                V[x] = 2;
            }
            else if (keypad[3]) {
                V[x] = 3;
            }
            else if (keypad[4]) {
                V[x] = 4;
            }
            else if (keypad[5]) {
                V[x] = 5;
            }
            else if (keypad[6]) {
                V[x] = 6;
            }
            else if (keypad[7]) {
                V[x] = 7;
            }
            else if (keypad[8]) {
                V[x] = 8;
            }
            else if (keypad[9]) {
                V[x] = 9;
            }
            else if (keypad[10]) {
                V[x] = 10;
            }
            else if (keypad[11]) {
                V[x] = 11;
            }
            else if (keypad[12]) {
                V[x] = 12;
            }
            else if (keypad[13]) {
                V[x] = 13;
            }
            else if (keypad[14]) {
                V[x] = 14;
            }
            else if (keypad[15]) {
                V[x] = 15;
            }
            else {
                pc -= 2;
            }

    }
    void OP_FX15 (){
        //set delay timer to vX
        uint16_t x = (opcode & 0x0F00u) >> 8u;
        delay_timer = V[x];
    }
    void OP_FX18 (){
        //set sound timer to vX
        uint16_t x = (opcode & 0x0F00u) >> 8u;
        sound_timer = V[x];
    }
    void OP_FX1E (){
        //add the value of vX to I
        uint16_t x = (opcode & 0x0F00u) >> 8u;
        I += V[x];
    }
    void OP_FX29() {
        //set I to the hexcode value corresponding the sprite
        uint16_t x = (opcode & 0x0F00u) >> 8u;
        uint16_t digit = V[x];

        I = FONTSET_START_ADDRESS + (5 * digit);
    }
    void OP_FX33() {
        //store binary coded decimal in I, I+1 and I+2
        uint16_t x = (opcode & 0x0F00u) >> 8u;

        memory[I]     = V[x] / 100;
        memory[I + 1] = (V[x] / 10) % 10;
        memory[I + 2] = V[x] % 10;

    }
    void OP_FX55 (){
        //fill memory I to I + X + 1 from register v0 to vX
        int16_t x = (opcode & 0x0F00u) >> 8u;
        for (int i = 0; i <= x; ++i) {
            memory[I + i] = V[i];
        }
        I += x + 1;
    }
    void OP_FX65 (){
        int16_t x = (opcode & 0x0F00u) >> 8u;
        //store memory I to I + X + 1 in register v0 to vX
        for (int i = 0; i <= x; ++i) {
             V[i] = memory[I + i];
        }
        I += x + 1;
    }
    void Cycle() {
        // Fetch
        opcode = (memory[pc] << 8u) | memory[pc + 1];
        
        // Increment the PC before we execute anything
        pc += 2;
        // Decode/Execute
        ((*this).*(table[(opcode & 0xF000u) >> 12u]))();

        // Decrement the delay timer if it's been set
        if (delay_timer > 0) {
            --delay_timer;
        }

        // Decrement the sound timer if it's been set
        if (sound_timer > 0) {
            --sound_timer;
        }
    
    }
    typedef void (Chip8::*Chip8Func)();
	Chip8Func table[0xF + 1];
	Chip8Func table0[0xE + 1];
	Chip8Func table8[0xE + 1];
	Chip8Func tableE[0xE + 1];
	Chip8Func tableF[0x65 + 1];

};
class Platform
{
public:
	Platform(char const* title, int windowWidth, int windowHeight, int textureWidth, int textureHeight)
	{
    
		SDL_Init(SDL_INIT_VIDEO);

		window = SDL_CreateWindow(title, 0, 0, windowWidth, windowHeight, SDL_WINDOW_SHOWN);

		renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

		texture = SDL_CreateTexture(
			renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, textureWidth, textureHeight);
	}

	~Platform()
	{
		
		SDL_DestroyTexture(texture);
		SDL_DestroyRenderer(renderer);
		SDL_DestroyWindow(window);
		SDL_Quit();
	}

	void Update(void const* buffer, int pitch)
	{
		SDL_UpdateTexture(texture, nullptr, buffer, pitch);
		SDL_RenderClear(renderer);
		SDL_RenderCopy(renderer, texture, nullptr, nullptr);
		SDL_RenderPresent(renderer);
	}

	bool ProcessInput(uint8_t* keys)
	{
		bool quit = false;

		SDL_Event event;

		while (SDL_PollEvent(&event))
		{
			switch (event.type)
			{
				case SDL_QUIT:
				{
					quit = true;
				} break;

				case SDL_KEYDOWN:
				{
					switch (event.key.keysym.sym)
					{
						case SDLK_ESCAPE:
						{
							quit = true;
						} break;

						case SDLK_x:
						{
							keys[0] = 1;
						} break;

						case SDLK_1:
						{
							keys[1] = 1;
						} break;

						case SDLK_2:
						{
							keys[2] = 1;
						} break;

						case SDLK_3:
						{
							keys[3] = 1;
						} break;

						case SDLK_q:
						{
							keys[4] = 1;
						} break;

						case SDLK_w:
						{
							keys[5] = 1;
						} break;

						case SDLK_e:
						{
							keys[6] = 1;
						} break;

						case SDLK_a:
						{
							keys[7] = 1;
						} break;

						case SDLK_s:
						{
							keys[8] = 1;
						} break;

						case SDLK_d:
						{
							keys[9] = 1;
						} break;

						case SDLK_z:
						{
							keys[0xA] = 1;
						} break;

						case SDLK_c:
						{
							keys[0xB] = 1;
						} break;

						case SDLK_4:
						{
							keys[0xC] = 1;
						} break;

						case SDLK_r:
						{
							keys[0xD] = 1;
						} break;

						case SDLK_f:
						{
							keys[0xE] = 1;
						} break;

						case SDLK_v:
						{
							keys[0xF] = 1;
						} break;
					}
				} break;

				case SDL_KEYUP:
				{
					switch (event.key.keysym.sym)
					{
						case SDLK_x:
						{
							keys[0] = 0;
						} break;

						case SDLK_1:
						{
							keys[1] = 0;
						} break;

						case SDLK_2:
						{
							keys[2] = 0;
						} break;

						case SDLK_3:
						{
							keys[3] = 0;
						} break;

						case SDLK_q:
						{
							keys[4] = 0;
						} break;

						case SDLK_w:
						{
							keys[5] = 0;
						} break;

						case SDLK_e:
						{
							keys[6] = 0;
						} break;

						case SDLK_a:
						{
							keys[7] = 0;
						} break;

						case SDLK_s:
						{
							keys[8] = 0;
						} break;

						case SDLK_d:
						{
							keys[9] = 0;
						} break;

						case SDLK_z:
						{
							keys[0xA] = 0;
						} break;

						case SDLK_c:
						{
							keys[0xB] = 0;
						} break;

						case SDLK_4:
						{
							keys[0xC] = 0;
						} break;

						case SDLK_r:
						{
							keys[0xD] = 0;
						} break;

						case SDLK_f:
						{
							keys[0xE] = 0;
						} break;

						case SDLK_v:
						{
							keys[0xF] = 0;
						} break;
					}
				} break;
			}
		}

		return quit;
	}

private:
	SDL_Window* window{};
	SDL_Renderer* renderer{};
	SDL_Texture* texture{};
};
int main(int argc, char** argv)
{
	if (argc != 4)
	{
		std::cerr << "Usage: " << argv[0] << " <Scale> <Delay> <ROM>\n";
		std::exit(EXIT_FAILURE);
	}

    int VIDEO_WIDTH = 64;
    int VIDEO_HEIGHT = 32;
	int videoScale = std::stoi(argv[1]);
	int cycleDelay = std::stoi(argv[2]);
	char const* romFilename = argv[3];

	Platform platform("CHIP-8 Emulator", VIDEO_WIDTH * videoScale, VIDEO_HEIGHT * videoScale, VIDEO_WIDTH, VIDEO_HEIGHT);

	Chip8 chip8;
	chip8.LoadROM(romFilename);

	int videoPitch = sizeof(chip8.gfx[0]) * VIDEO_WIDTH;

	auto lastCycleTime = std::chrono::high_resolution_clock::now();
	bool quit = false;

	while (!quit)
	{
		quit = platform.ProcessInput(chip8.keypad);

		auto currentTime = std::chrono::high_resolution_clock::now();
		float dt = std::chrono::duration<float, std::chrono::milliseconds::period>(currentTime - lastCycleTime).count();

		if (dt > cycleDelay)
		{
			lastCycleTime = currentTime;

			chip8.Cycle();

			platform.Update(chip8.gfx, videoPitch);
		}
	}

	return 0;
}

