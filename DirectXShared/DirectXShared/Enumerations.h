#pragma once

enum Type
{
	TMeshCreate,
	TMeshUpdate,
	TAddedVertex,
	TVertexUpdate,
	TNormalUpdate,
	TUVUpdate,
	TtransformUpdate,
	TCameraUpdate,
	TLightCreate,
	TLightUpdate,
	TMeshDestroyed,
	TMaterialUpdate,
	TMaterialChanged,
	TAmount
};