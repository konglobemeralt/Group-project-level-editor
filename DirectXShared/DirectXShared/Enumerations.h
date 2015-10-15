#pragma once

enum Type
{
	TMeshCreate,
	TMeshUpdate,
	TVertexUpdate,
	TNormalUpdate,
	TUVUpdate,
	TtransformUpdate,
	TCameraUpdate,
	TLightCreate,
	TLightUpdate,
	TMeshDestroyed,
	TLightDestroyed,
	TAmount
};