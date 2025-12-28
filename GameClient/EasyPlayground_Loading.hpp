
void EasyPlayground::DeInit()
{
	// Network
	if (network)
		delete network;
	network = nullptr;

	// Players and Main Player
	for (Player* p : players)
		delete p;
	players.clear();
	player = nullptr;

	// Entities
	for (auto& [name, entity] : entities)
		delete entity;
	entities.clear();

	// HDRs
	if (hdr)
		delete hdr;
	hdr = nullptr;

	// Models
	for (auto& [name, model] : models)
		delete model;
	models.clear();

	// Chunks
	for (Chunk* c : chunks)
		delete c;
	chunks.clear();

	// Camera
	if (camera)
		delete camera;
	camera = nullptr;

	// Engine
	EasyIO::DeInit();
	HDR::DeInit();
	SkyboxRenderer::DeInit();
	ChunkRenderer::DeInit();
	ModelRenderer::DeInit();
	DebugRenderer::DeInit();

	bm = nullptr;
}

bool EasyPlayground::Init(EasyBufferManager* bm)
{
	EasyPlayground::DeInit();

	this->bm = bm;

	// Inputs
	EasyIO::Init();
	EasyKeyboard::AddListener(this);
	EasyMouse::AddListener(this);

	// Shaders
	ReloadShaders();

	// Assets
	ReloadAssets();

	// ImGUI
	ImGUI_Init();

	return true;
}

void EasyPlayground::ReloadAssets()
{
	if (network)
		delete network;
	network = nullptr;

	for (Player* p : players)
		delete p;
	players.clear();
	player = nullptr;

	for (auto& [name,model] : models)
		delete model;
	models.clear();

	if (camera)
		delete camera;
	camera = nullptr;

	if (hdr)
		delete hdr;
	hdr = nullptr;
	
	Model("MainCharacter") = EasyModel::LoadModel(
#if 1
		GetRelPath("res/models/Kachujin G Rosales Skin.fbx"),
		{
			GetRelPath("res/models/C_Idle_v2.fbx"),
			GetRelPath("res/models/C_Back_v2.fbx"),
			GetRelPath("res/models/C_Run_v2.fbx"),
			GetRelPath("res/models/C_StrafeRight_v2.fbx"),
			GetRelPath("res/models/C_StrafeLeft_v2.fbx"),
			GetRelPath("res/models/C_TurnRight_v2.fbx"),
			GetRelPath("res/models/C_TurnLeft_v2.fbx"),
#else
		GetRelPath("res/models/Kachujin G Rosales Skin.fbx"),
		{
			GetRelPath("res/models/Character Idle.fbx"),
			GetRelPath("res/models/Character Run Backward.fbx"),
			GetRelPath("res/models/Character Run Forward.fbx"),
			GetRelPath("res/models/Character Strafe Right.fbx"),
			GetRelPath("res/models/Character Strafe Left.fbx"),
		#if 1
			#if 1
			GetRelPath("res/models/Character Idle Turn Right Sans.fbx"),
			GetRelPath("res/models/Character Idle Turn Left Sans.fbx"),
			#else
			GetRelPath("res/models/Character Idle Turn Right.fbx"),
			GetRelPath("res/models/Character Idle Turn Left.fbx"),
			#endif
		#else
			GetRelPath("res/models/Character Idle.fbx"),
			GetRelPath("res/models/Character Idle.fbx"),
		#endif
#endif
		}, glm::vec3(0.0082f) // Y = 1.70m
		);


#ifndef FIRE_LIGHTWEIGHT
	Model("1x1cube.dae") = EasyModel::LoadModel(GetRelPath("res/models/1x1cube.dae"));
	Model("items.dae") = EasyModel::LoadModel(GetRelPath("res/models/items.dae"));
	Model("Buildings.dae") = EasyModel::LoadModel(GetRelPath("res/models/Buildings.dae"));
	Model("Walls.dae") = EasyModel::LoadModel(GetRelPath("res/models/Walls.dae"));
#endif

#ifndef FIRE_LIGHTWEIGHT
	entities.emplace("1x1x1_cube", new EasyEntity(Model("1x1cube.dae"), { {-1,0,0}, {}, {1,1,1} }));
#endif
	
	network = new ClientNetwork(bm, this);

	camera = new FRCamera();

	hdr = new HDR("defaultLightingHDR");

	ReGenerateMap();
}

void EasyPlayground::ReloadShaders()
{
	std::cout << "[EasyPlayground] ReloadShaders - Reloading shaders...\n";

	HDR::Init();
	DebugRenderer::Init();
	ModelRenderer::Init();
	SkyboxRenderer::Init();
	ChunkRenderer::Init();
}

void EasyPlayground::ReGenerateMap()
{
	std::cout << "[EasyPlayground] ReGenerateMap - Regenerating map...\n";

	// Cleanup
	for (size_t i = 0u; i < chunks.size(); i++)
		delete chunks.at(i);
	chunks.clear();

	// New Seed
	seed = (int)(glm::linearRand(0.7f, 1.3f) * 10000u);

	// Generate Center
	Chunk::GenerateChunkAt(chunks, { 0,0 }, seed);

	// Make 3x3
	//Chunk::GenerateChunkAt(chunks, { 1,0 }, seed);
	//Chunk::GenerateChunkAt(chunks, { -1,0 }, seed);
	//Chunk::GenerateChunkAt(chunks, { 0,1 }, seed);
	//Chunk::GenerateChunkAt(chunks, { 0,-1 }, seed);
	//Chunk::GenerateChunkAt(chunks, { 1,1 }, seed);
	//Chunk::GenerateChunkAt(chunks, { 1,-1 }, seed);
	//Chunk::GenerateChunkAt(chunks, { -1,1 }, seed);
	//Chunk::GenerateChunkAt(chunks, { -1,-1 }, seed);

#if 1
	// Generate some other

#endif

#if 0
	// Add lake
	Chunk::AddLake(chunks, glm::vec2(0.f, 0.f), 30.0f);   // small pond
	Chunk::AddLake(chunks, glm::vec2(100.f, 0.f), 60.0f); // big lake
#endif

	// Load all to GPU
	for (Chunk* c : chunks)
		c->LoadToGPU();
}
