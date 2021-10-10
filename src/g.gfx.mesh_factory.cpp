#include "g.gfx.h"

#ifdef _WIN32
# define strtok_r strtok_s
#endif // _WIN32


using namespace g::gfx;


mesh<vertex::pos> mesh_factory::slice_cube(unsigned slices)
{
    mesh<vertex::pos> c;
    std::vector<vertex::pos> verts;
    std::vector<uint32_t> indices;
    glGenBuffers(2, &c.vbo);

    const float h = 0.5 * sqrt(3);
    float dz = (2.f * h) / static_cast<float>(slices);

    for (;slices--;)
    {
        auto n = verts.size();
        indices.push_back(n + 0);
        indices.push_back(n + 3);
        indices.push_back(n + 2);
        indices.push_back(n + 0);
        indices.push_back(n + 2);
        indices.push_back(n + 1);

        auto z = dz * slices;
        verts.push_back({{-h, h, -h + z}});
        verts.push_back({{ h, h, -h + z}});
        verts.push_back({{ h,-h, -h + z}});
        verts.push_back({{-h,-h, -h + z}});

    }

    c.set_vertices(verts);
    c.set_indices(indices);

    return c;
}

mesh<vertex::pos_uv_norm> mesh_factory::cube()
{
    mesh<vertex::pos_uv_norm> c;
    glGenBuffers(1, &c.vbo);
    c.set_vertices({
        // +x face
        {{ 1,-1, 1}, {1, 1}, {1, 0, 0}},
        {{ 1, 1, 1}, {0, 1}, {1, 0, 0}},
        {{ 1, 1,-1}, {0, 0}, {1, 0, 0}},
        {{ 1,-1,-1}, {1, 0}, {1, 0, 0}},

        // -x face
        {{-1,-1, 1}, {1, 1}, {-1, 0, 0}},
        {{-1, 1, 1}, {0, 1}, {-1, 0, 0}},
        {{-1, 1,-1}, {0, 0}, {-1, 0, 0}},
        {{-1,-1,-1}, {1, 0}, {-1, 0, 0}},

        // +y face
        {{-1, 1, 1}, {1, 1}, {0, 1, 0}},
        {{ 1, 1, 1}, {0, 1}, {0, 1, 0}},
        {{ 1, 1,-1}, {0, 0}, {0, 1, 0}},
        {{-1, 1,-1}, {1, 0}, {0, 1, 0}},

        // -y face
        {{-1,-1, 1}, {1, 1}, {0,-1, 0}},
        {{ 1,-1, 1}, {0, 1}, {0,-1, 0}},
        {{ 1,-1,-1}, {0, 0}, {0,-1, 0}},
        {{-1,-1,-1}, {1, 0}, {0,-1, 0}},

        // +z face
        {{-1, 1, 1}, {1, 1}, {0, 0, 1}},
        {{ 1, 1, 1}, {0, 1}, {0, 0, 1}},
        {{ 1,-1, 1}, {0, 0}, {0, 0, 1}},
        {{-1,-1, 1}, {1, 0}, {0, 0, 1}},

        // -z face
        {{-1, 1,-1}, {1, 1}, {0, 0, -1}},
        {{ 1, 1,-1}, {0, 1}, {0, 0, -1}},
        {{ 1,-1,-1}, {0, 0}, {0, 0, -1}},
        {{-1,-1,-1}, {1, 0}, {0, 0, -1}},
    });

    glGenBuffers(1, &c.ibo);
    c.set_indices({
        (0) + 2, (0) + 3, (0) + 0, (0) + 1, (0) + 2, (0) + 0,
        (4) + 0, (4) + 3, (4) + 2, (4) + 0, (4) + 2, (4) + 1,
        (8) + 0, (8) + 3, (8) + 2, (8) + 0, (8) + 2, (8) + 1,
        (12) + 2, (12) + 3, (12) + 0, (12) + 1, (12) + 2, (12) + 0,
        (16) + 2, (16) + 3, (16) + 0, (16) + 1, (16) + 2, (16) + 0,
        (20) + 0, (20) + 3, (20) + 2, (20) + 0, (20) + 2, (20) + 1,
    });

    return c;
}


mesh<vertex::pos_uv_norm> mesh_factory::plane()
{
    mesh<vertex::pos_uv_norm> p;
    glGenBuffers(2, &p.vbo);

    p.set_vertices({
        {{-1,-1, 0}, {1, 0}, {0, 0, 1}},
        {{ 1,-1, 0}, {0, 0}, {0, 0, 1}},
        {{ 1, 1, 0}, {0, 1}, {0, 0, 1}},
        {{-1, 1, 0}, {1, 1}, {0, 0, 1}},
    });

    return p;
}


// TODO: move all of this into its own file
enum obj_line_type {
    COMMENT = 0,
    POSITION,
    TEXTURE,
    NORMAL,
    PARAMETER,
    FACE,
    UNKNOWN,
};


struct obj_line {
    obj_line_type type;
    char str[1024];
    union {
        vec<3> position;
        vec<2> texture;
        vec<3> normal;
        vec<3> parameter;
        struct {
            uint32_t pos_idx[3], tex_idx[3], norm_idx[3];
        } face;
    };
};

//------------------------------------------------------------------------------
static int get_line(int fd, char* line)
{
    int size = 0;
    while(read(fd, line + size, 1))
    {
        if(line[size] == '\n') break;
        ++size;
    }

    line[size] = '\0';

    return size;
}

//------------------------------------------------------------------------------
bool parse_line(int fd, obj_line& line)
{
    char *save_ptr = nullptr;
    line.type = UNKNOWN;

    if(get_line(fd, line.str) == 0)
    {
        return false;
    }

    char* token = strtok_r(line.str, " ", &save_ptr);
    if(!token) return false;

    // Determine the tag of the line
    const char* tag[] = { R"(#)", R"(v)", R"(vt)", R"(vn)", R"(vp)", R"(f)", nullptr };
    for(int i = 0; tag[i]; ++i)
    {
        if(strcmp(tag[i], token) == 0)
        {
            line.type = (obj_line_type)i;
            break;
        }
    }

    float* v;
    int vec_size = 0;
    switch (line.type)
    {
        case COMMENT:
            break;
        case POSITION:
            v = line.position.v;
            vec_size = 3;
            break;
        case TEXTURE:
            v = line.texture.v;
            vec_size = 2;
            break;
        case NORMAL:
            v = line.normal.v;
            vec_size = 3;
            break;
        case PARAMETER:
            v = line.parameter.v;
            vec_size = 3;
            break;
        case FACE:
        memset(&line.face, 0, sizeof(line.face));
        for(int i = 0; i < 3; ++i)
        {
            token = strtok_r(nullptr, " ", &save_ptr);
            if(!token) break;

            char* idx_token = token;
            for(int j = strlen(idx_token); j--;)
            {
                if(idx_token[j] == '/') idx_token[j] = '\0';
            }

            for(int j = 0; j < 3; ++j)
            {
                if(*idx_token != '\0') switch (j)
                {
                    case 0:
                        sscanf(idx_token, "%d", &line.face.pos_idx[i]);
                        break;
                    case 1:
                        sscanf(idx_token, "%d", &line.face.tex_idx[i]);
                        break;
                    case 2:
                        sscanf(idx_token, "%d", &line.face.norm_idx[i]);
                        break;
                }

                idx_token += strlen(idx_token) + 1;
            }

            // printf("%d/%d/%d\n", line.face.pos_idx[i], line.face.tex_idx[i], line.face.norm_idx[i]);
        }
            break;
        default:;
    }

    // Read the vector selected above
    for(int i = 0; i < vec_size; ++i)
    {
        token = strtok_r(nullptr, " ", &save_ptr);
        // printf("type: %d TOK: '%s' %d\n", line.type, token, vec_size);
        sscanf(token, "%f", v + i);
    }

    return true;
}

//------------------------------------------------------------------------------
mesh<vertex::pos_uv_norm> mesh_factory::from_obj(const std::string& path)
{
    mesh<vertex::pos_uv_norm> mesh;
    std::unordered_map<std::string, uint32_t> index_map;

    std::vector<vec<3>> positions, normals, params;
    std::vector<vec<2>> tex_coords;
    std::vector<uint32_t> indices;
    std::vector<vertex::pos_uv_norm> vertices;

    auto fd = open(path.c_str(), O_RDONLY);

    obj_line l = {};
    while(parse_line(fd, l))
    {
        switch (l.type)
        {
            case COMMENT:
                // printf("%s\n", l.str);
                break;
            case POSITION:
            {
                positions.push_back({ l.position[0], l.position[1], l.position[2] });
            }
                break;
            case TEXTURE:
            {
                tex_coords.push_back({ l.texture[0], l.texture[1] });
            }
                break;
            case NORMAL:
            {
                normals.push_back({ l.normal[0], l.normal[1], l.normal[2] });
            }
                break;
            case PARAMETER:
            {
                params.push_back({ l.parameter[0], l.parameter[1], l.parameter[2] });
            }
                break;
            case FACE:
            {
                for(int i = 0; i < 3; ++i)
                {
                    vertex::pos_uv_norm v = {};
                    if(l.face.pos_idx[i])  v.position = positions[l.face.pos_idx[i] - 1];
                    if(l.face.tex_idx[i])  v.uv = tex_coords[l.face.tex_idx[i] - 1];
                    if(l.face.norm_idx[i]) v.normal = normals[l.face.norm_idx[i] - 1];

                    char token[21] = {};

                    snprintf(token, sizeof(token), "%u/%u/%u",
                             l.face.pos_idx[i],
                             l.face.tex_idx[i],
                             l.face.norm_idx[i]
                    );

                    std::string vert_token(token);

                    if (index_map.count(vert_token) == 0)
                    {
                        vertices.push_back(v);
                        index_map[vert_token] = (uint32_t)(vertices.size() - 1);
                    }

                    indices.push_back(index_map[vert_token]);
                }
            }
                break;
            case UNKNOWN:
                break;
        }
    }

    std::reverse(indices.begin(), indices.end());

    glGenBuffers(2, &mesh.vbo);
    mesh.set_vertices(vertices);
    mesh.set_indices(indices);
    // compute_tangents();

    return mesh;
}
