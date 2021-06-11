#pragma once

#include "ir.hpp"
#include "ast.hpp"

struct Typer {
};

void typeCheckFilePrepass(ASTFile *file);
