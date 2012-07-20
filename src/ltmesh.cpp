#include "lt.h"

LT_INIT_IMPL(ltmesh);

LTMesh::LTMesh(LTMesh *mesh) {
    dimensions = mesh->dimensions;
    has_colors = mesh->has_colors;
    has_normals = mesh->has_normals;
    has_texture_coords = mesh->has_texture_coords;
    texture = mesh->texture;
    draw_mode = mesh->draw_mode;

    stride = mesh->stride;
    size = mesh->size;
    data = malloc(stride * size);
    memcpy(data, mesh->data, stride * size);

    dirty = true;
    vertbuf = ltGenVertBuffer();
}

LTMesh::LTMesh(int dims, bool has_col, bool has_norm, bool has_tex_coords, LTImage *tex, LTDrawMode mode, void* dat, int sz) {
    dimensions = dims;
    has_colors = has_col;
    has_normals = has_norm;
    has_texture_coords = has_tex_coords;
    texture = tex;
    draw_mode = mode;
    data = dat;
    size = sz;
    dirty = true;
}

LTMesh::~LTMesh() {
    if (data != NULL) {
        free(data);
    }
    if (vertbuf != 0) {
        ltDeleteVertBuffer(vertbuf);
    }
}

void LTMesh::setup() {
    stride = dimensions * 4; // 4 bytes per vertex coord
    stride += has_colors ? 4 : 0; // 1 byte per channel (r, g, b, a = 4 total)
    stride += has_normals ? 4 : 0; // 1 byte per normal component + 1 extra to keep 4-byte alignment
    stride += has_texture_coords ? 4 : 0; // 2 bytes per texture coordinate (u + v) = 4 total
    vertbuf = ltGenVertBuffer();
}

void LTMesh::draw() {
    ensure_buffer_uptodate();
    ltBindVertBuffer(vertbuf);
    int offset = 0;
    ltVertexPointer(dimensions, LT_VERT_DATA_TYPE_FLOAT, stride, (void*)offset);
    offset += dimensions * 4;
    if (has_colors) {
        ltEnableColorArrays();
        ltColorPointer(4, LT_VERT_DATA_TYPE_UBYTE, stride, (void*)offset);
        offset += 4;
    }
    if (has_normals) {
        ltEnableNormalArrays();
        ltNormalPointer(LT_VERT_DATA_TYPE_BYTE, stride, (void*)offset);
        offset += 4;
    }
    if (texture != NULL) {
        ltEnableTexture(texture->texture_id);
        ltTexCoordPointer(2, LT_VERT_DATA_TYPE_SHORT, stride, (void*)offset);
    } else {
        ltDisableTextures();
    }
    offset += (has_texture_coords ? 4 : 0);
    ltDrawArrays(draw_mode, 0, size);
    if (has_normals) {
        ltDisableNormalArrays();
    }
    if (has_colors) {
        ltDisableColorArrays();
        ltColorPointer(4, LT_VERT_DATA_TYPE_FLOAT, 0, 0); // XXX necessary?
        ltRestoreTint();
    }
}

void LTMesh::stretch(LTfloat px, LTfloat py, LTfloat pz,
    LTfloat left, LTfloat right, LTfloat down, LTfloat up, LTfloat backward, LTfloat forward)
{
    char *ptr = (char*)data;
    for (int i = 0; i < size; i++) {
        LTfloat *x = (LTfloat*)ptr;
        LTfloat *y = x + 1;
        if (*x < px) {
            *x -= left;
        } else {
            *x += right;
        }
        if (*y < py) {
            *y -= down;
        } else {
            *y += up;
        }
        if (dimensions > 2) {
            LTfloat *z = x + 2;
            if (*z < pz) {
                *z -= backward;
            } else {
                *z += forward;
            }
        }
        ptr += stride;
    }

    dirty = true;
}

void LTMesh::shift(LTfloat sx, LTfloat sy, LTfloat sz) {
    char *ptr = (char*)data;
    for (int i = 0; i < size; i++) {
        LTfloat *x = (LTfloat*)ptr;
        LTfloat *y = x + 1;
        *x += sx;
        *y += sy;
        if (dimensions > 2) {
            LTfloat *z = x + 2;
            *z += sz;
        }
        ptr += stride;
    }

    dirty = true;
}

void LTMesh::merge(LTMesh *mesh) {
    assert(mesh->dimensions == dimensions);
    assert(mesh->has_colors == has_colors);
    assert(mesh->has_normals == has_normals);
    assert(mesh->has_texture_coords == has_texture_coords);
    assert(mesh->stride == stride);
    assert(mesh->draw_mode == draw_mode);
    assert(draw_mode == LT_DRAWMODE_TRIANGLES); // Others NYI

    data = realloc(data, stride * (size + mesh->size));
    memcpy(((char*)data) + stride * size, mesh->data, stride * mesh->size);
    size += mesh->size;

    dirty = true;
}

void LTMesh::ensure_buffer_uptodate() {
    if (dirty) {
        ltBindVertBuffer(vertbuf);
        ltStaticVertBufferData(size * stride, data);
        dirty = false;
    }
}

void LTMesh::print() {
    printf("     X     Y     Z");
    if (has_colors) {
        printf("  R  G  B  A");
    }
    if (has_normals) {
        printf("    NX    NY    NZ");
    }
    if (has_texture_coords) {
        printf("     U     V");
    }
    printf("\n");
    void *ptr = data;
    for (int i = 0; i < size; i++) {
        LTfloat *vptr = (LTfloat*)ptr;
        printf(" %5.2f %5.2f %5.2f", (double)vptr[0], (double)vptr[1], (double)vptr[2]);
        lt_incr_ptr(ptr, 3 * 4);
        if (has_colors) {
            LTubyte *cptr = (LTubyte*)ptr;
            printf(" %2X %2X %2X %2X", cptr[0], cptr[1], cptr[2], cptr[3]);
            lt_incr_ptr(ptr, 4);
        }
        if (has_normals) {
            LTbyte *nptr = (LTbyte*)ptr;
            printf(" %5.2f %5.2f %5.2f", ((double)nptr[0])/127.0f, ((double)nptr[1])/127.0f, ((double)nptr[2])/127.0f);
            lt_incr_ptr(ptr, 4);
        }
        if (has_texture_coords) {
            LTshort *tptr = (LTshort*)ptr;
            printf(" %5.2f %5.2f", (double)tptr[0]/(double)LT_MAX_TEX_COORD, (double)tptr[1]/(double)LT_MAX_TEX_COORD);
            lt_incr_ptr(ptr, 4);
        }
        printf("\n");
    }
}

static int clone_mesh(lua_State *L) {
    ltLuaCheckNArgs(L, 1);
    LTMesh *mesh = lt_expect_LTMesh(L, 1);
    new (lt_alloc_LTMesh(L)) LTMesh(mesh);
    return 1;
}

static int stretch_mesh(lua_State *L) {
    ltLuaCheckNArgs(L, 10);
    LTMesh *mesh = lt_expect_LTMesh(L, 1);
    LTfloat px = luaL_checknumber(L, 2);
    LTfloat py = luaL_checknumber(L, 3);
    LTfloat pz = luaL_checknumber(L, 4);
    LTfloat left = luaL_checknumber(L, 5);
    LTfloat right = luaL_checknumber(L, 6);
    LTfloat down = luaL_checknumber(L, 7);
    LTfloat up = luaL_checknumber(L, 8);
    LTfloat backward = luaL_checknumber(L, 9);
    LTfloat forward = luaL_checknumber(L, 10);
    mesh->stretch(px, py, pz, left, right, down, up, backward, forward);
    lua_pushvalue(L, 1);
    return 1;
}

static int shift_mesh(lua_State *L) {
    ltLuaCheckNArgs(L, 4);
    LTMesh *mesh = lt_expect_LTMesh(L, 1);
    LTfloat sx = luaL_checknumber(L, 2);
    LTfloat sy = luaL_checknumber(L, 3);
    LTfloat sz = luaL_checknumber(L, 4);
    mesh->shift(sx, sy, sz);
    lua_pushvalue(L, 1);
    return 1;
}

static int merge_mesh(lua_State *L) {
    ltLuaCheckNArgs(L, 2);
    LTMesh *mesh1 = lt_expect_LTMesh(L, 1);
    LTMesh *mesh2 = lt_expect_LTMesh(L, 2);

    if (mesh1->dimensions != mesh2->dimensions) {
        return luaL_error(L, "Mesh merge error: incompatible dimensions");
    }
    if (mesh1->has_colors != mesh2->has_colors) {
        return luaL_error(L, "Mesh merge error: one has colors, the other doesn't");
    }
    if (mesh1->has_normals != mesh2->has_normals) {
        return luaL_error(L, "Mesh merge error: one has normals, the other doesn't");
    }
    if (mesh1->has_texture_coords != mesh2->has_texture_coords) {
        return luaL_error(L, "Mesh merge error: one has texture coords, the other doesn't");
    }
    if (mesh1->stride != mesh2->stride) {
        return luaL_error(L, "Mesh merge error: incompatible strides");
    }
    if (mesh1->draw_mode != LT_DRAWMODE_TRIANGLES || mesh2->draw_mode != LT_DRAWMODE_TRIANGLES) {
        return luaL_error(L, "Mesh merge error: sorry, only triangle meshes can be merged");
    }
    mesh1->merge(mesh2);
    lua_pushvalue(L, 1);
    return 1;
}

LT_REGISTER_TYPE(LTMesh, "lt.Mesh", "lt.SceneNode")
LT_REGISTER_METHOD(LTMesh, Clone, clone_mesh)
LT_REGISTER_METHOD(LTMesh, Stretch, stretch_mesh)
LT_REGISTER_METHOD(LTMesh, Shift, shift_mesh)
LT_REGISTER_METHOD(LTMesh, Merge, merge_mesh)
