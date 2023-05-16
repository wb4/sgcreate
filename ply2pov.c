
#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#define LINE_MAX 256


typedef struct header_tag {
  size_t vertex_count;
  size_t face_count;
} header_t;


typedef struct vertex_tag {
  float x;
  float y;
  float z;
} vertex_t;


typedef struct triangle_tag {
  size_t vertices[3];
} triangle_t;


void remove_newline(char *str) {
  size_t start;

  for (start = strlen(str);  start > 0 && str[start-1] == '\n';  --start) {
    str[start-1] = '\0';
  }
}


int ascii_to_size_t(const char *ascii, size_t *result) {
  char *end;
  unsigned long temp;

  temp = strtoul(ascii, &end, 10);

  if (*end) {
    return -1;
  }

  if (errno == ERANGE) {
    return -1;
  }

  *result = (size_t) temp;
  if (*result != temp) {
    errno = ERANGE;
    return -1;
  }

  return 0;
}


int ascii_to_float(const char *ascii, float *result) {
  char *end;
  float temp;

  errno = 0;

  temp = strtof(ascii, &end);

  if (*end) {
    return -1;
  }

  if ((temp == HUGE_VALF || temp == -HUGE_VALF || temp == 0.0f) && errno == ERANGE) {
    return -1;
  }

  *result = temp;

  return 0;
}


int extract_end_size_t(size_t *dest, const char *str) {
  size_t start;

  for (start = strlen(str);  start > 0;  --start) {
    if (!isdigit(str[start-1])) {
      break;
    }
  }

  if (!str[start]) {
    fprintf(stderr, "No size_t at the end of \"%s\"\n", str);
    return -1;
  }

  if (ascii_to_size_t(&str[start], dest) == -1) {
    perror("ascii_to_size_t()");
    return -1;
  }

  return 0;
}


int read_line(FILE *in, char *dest, size_t length) {
  if (!fgets(dest, length, in)) {
    if (ferror(in)) {
      perror("read_line()");
    } else {
      fputs("Unexpected EOF\n", stderr);
    }
    return -1;
  }

  remove_newline(dest);

  return 0;
}


int read_header(FILE *in, header_t *header) {
  char line[LINE_MAX];
  int have_vertex_count = 0;
  int have_face_count = 0;

  const char *format_line = "format ascii";
  const char *vertex_line = "element vertex ";
  const char *face_line = "element face ";

  if (read_line(in, line, LINE_MAX) == -1) return -1;
  if (strcmp(line, "ply")) {
    fputs("This is not a PLY file.\n", stderr);
    return -1;
  }

  if (read_line(in, line, LINE_MAX) == -1) return -1;
  if (strncmp(line, format_line, strlen(format_line))) {
    fputs("This is not an ASCII file.  I don't support binary.\n", stderr);
    return -1;
  }

  for (;;) {
    if (read_line(in, line, LINE_MAX) == -1) return -1;

    if (!strcmp(line, "end_header")) {
      break;
    }

    if (!strncmp(line, vertex_line, strlen(vertex_line))) {
      if (extract_end_size_t(&header->vertex_count, line) == -1) {
        return -1;
      }
      have_vertex_count = 1;
    }

    if (!strncmp(line, face_line, strlen(face_line))) {
      if (extract_end_size_t(&header->face_count, line) == -1) {
        return -1;
      }
      have_face_count = 1;
    }
  }

  if (!have_vertex_count) {
    fputs("There was no vertex count in the header.\n", stderr);
    return -1;
  }
  if (!have_face_count) {
    fputs("There was no face count in the header.\n", stderr);
    return -1;
  }

  return 0;
}


int next_word(const char **ptr, char *dest, size_t length) {
  size_t i;

  while (isspace(**ptr)) ++*ptr;

  if (!**ptr) {
    fprintf(stderr, "No words in \"%s\"\n", *ptr);
    return -1;
  }

  for (i = 0;  i < length - 1;  ++i, ++*ptr) {
    if (isspace(**ptr) || !**ptr) {
      break;
    }

    dest[i] = **ptr;
  }

  dest[i] = '\0';

  return 0;
}


int next_size_t(const char **ptr, size_t *dest) {
  char word[LINE_MAX];

  if (next_word(ptr, word, LINE_MAX) == -1) return -1;

  if (ascii_to_size_t(word, dest) == -1) {
    fprintf(stderr, "ascii_to_size_t() on \"%s\"\n", word);
    return -1;
  }

  return 0;
}


int next_float(const char **ptr, float *dest) {
  char word[LINE_MAX];

  if (next_word(ptr, word, LINE_MAX) == -1) return -1;

  if (ascii_to_float(word, dest) == -1) {
    fprintf(stderr, "ascii_to_float() on \"%s\"\n", word);
    return -1;
  }

  return 0;
}


void transform_vertex(vertex_t *v) {
  float temp;

  temp = v->y;
  v->y = v->z;
  v->z = temp;
}


int read_vertices(FILE *in, vertex_t *vertices, size_t count) {
  char line[LINE_MAX];
  const char *ptr;
  size_t i;


  for (i = 0;  i < count;  ++i) {
    if (read_line(in, line, LINE_MAX) == -1) return -1;

    ptr = line;

    if (next_float(&ptr, &vertices[i].x) == -1) return -1;
    if (next_float(&ptr, &vertices[i].y) == -1) return -1;
    if (next_float(&ptr, &vertices[i].z) == -1) return -1;

    transform_vertex(&vertices[i]);
  }

  return 0;
}


int write_ini_header(FILE *out, const char *mesh_name) {
  if (fprintf(out, "#declare %s = mesh {\n", mesh_name) < 0) {
    perror("writing INI header");
    return -1;
  }

  return 0;
}


int write_ini_footer(FILE *out) {
  if (fputs("}\n", out) == EOF) {
    perror("writing INI footer");
    return -1;
  }

  return 0;
}


int extract_face(const char *str, size_t *indices, size_t *index_count) {
  const char *ptr;
  size_t i;

  ptr = str;

  if (next_size_t(&ptr, index_count) == -1) return -1;

  for (i = 0;  i < *index_count;  ++i) {
    if (next_size_t(&ptr, &indices[i]) == -1) return -1;
  }

  return 0;
}


int find_vertex(const vertex_t **vertex, size_t index, const vertex_t *list, size_t count) {
  if (index >= count) {
    fprintf(stderr, "find_vertex(): index %lu out of range of list of size %lu\n", index, count);
    return -1;
  }

  *vertex = &list[index];

  return 0;
}


void vertex_sub(vertex_t *dest, const vertex_t *v1, const vertex_t *v2) {
  dest->x = v1->x - v2->x;
  dest->y = v1->y - v2->y;
  dest->z = v1->z - v2->z;
}


void vertex_cross(vertex_t *dest, const vertex_t *v1, const vertex_t *v2) {
  dest->x = v1->y * v2->z - v1->z * v2->y;
  dest->y = v1->z * v2->x - v1->x * v2->z;
  dest->z = v1->x * v2->y - v1->y * v2->x;
}


int should_omit_index(int *should_omit, size_t index, const size_t *indices, const vertex_t *vertices, size_t vertex_count) {
  size_t start_i = 0;
  size_t end_i = 1;

  const vertex_t *start_v;
  const vertex_t *end_v;
  const vertex_t *last_v;
  const vertex_t *omitting_v;

  vertex_t line;
  vertex_t last_line;
  vertex_t omitting_line;

  vertex_t cross_1;
  vertex_t cross_2;

  float comp_1;
  float comp_2;

  if (index <= start_i) ++start_i;
  if (index <= end_i) ++end_i;

  if (find_vertex(&start_v, indices[start_i], vertices, vertex_count) == -1) return -1;
  if (find_vertex(&end_v, indices[end_i], vertices, vertex_count) == -1) return -1;
  if (find_vertex(&last_v, indices[3], vertices, vertex_count) == -1) return -1;
  if (find_vertex(&omitting_v, indices[index], vertices, vertex_count) == -1) return -1;

  vertex_sub(&line, end_v, start_v);
  vertex_sub(&last_line, last_v, start_v);
  vertex_sub(&omitting_line, omitting_v, start_v);

  vertex_cross(&cross_1, &last_line, &line);
  vertex_cross(&cross_2, &omitting_line, &line);

  if (fabsf(cross_1.x) > fabsf(cross_1.y)) {
    if (fabsf(cross_1.x) > fabsf(cross_1.z)) {
      comp_1 = cross_1.x;
      comp_2 = cross_2.x;
    } else {
      comp_1 = cross_1.z;
      comp_2 = cross_2.z;
    }
  } else {
    if (fabsf(cross_1.y) > fabsf(cross_1.z)) {
      comp_1 = cross_1.y;
      comp_2 = cross_2.y;
    } else {
      comp_1 = cross_1.z;
      comp_2 = cross_2.z;
    }
  }

  *should_omit = (comp_1 > 0) ^ (comp_2 > 0);

  return 0;
}


int find_omitted_index(size_t *omitted, const size_t *indices, const vertex_t *vertices, size_t vertex_count) {
  int should_omit;

  for (*omitted = 0;  *omitted < 3;  ++*omitted) {
    if (should_omit_index(&should_omit, *omitted, indices, vertices, vertex_count) == -1) return -1;
    if (should_omit) {
      break;
    }
  }

  return 0;
}


int find_complementary_triangle(triangle_t *triangle, const size_t *indices, const vertex_t *vertices, size_t vertex_count) {
  size_t i;
  size_t j;
  size_t skip_index;

  if (find_omitted_index(&skip_index, indices, vertices, vertex_count) == -1) return -1;

  for (i = 0, j = 0;  i < 3;  ++i, ++j) {
    if (j == skip_index) {
      ++j;
    }
    triangle->vertices[i] = indices[j];
  }

  return 0;
}


int face_to_triangles(const size_t *indices, size_t index_count, triangle_t *triangles, size_t *triangle_count, const vertex_t *vertices, size_t vertex_count) {
  size_t i;

  if (index_count < 3) {
    fprintf(stderr, "face_to_triangles(): Fewer than 3 indexes given: %lu\n", index_count);
    return -1;
  }

  if (index_count > 4) {
    fprintf(stderr, "face_to_triangles(): Greater than 4 indexes given: %lu\n", index_count);
    return -1;
  }

  *triangle_count = index_count - 2;

  for (i = 0;  i < 3;  ++i) {
    triangles[0].vertices[i] = indices[i];
  }

  if (index_count == 4) {
    if (find_complementary_triangle(&triangles[1], indices, vertices, vertex_count) == -1) return -1;
  }

  return 0;
}


void dump_face(const size_t *indices, size_t count) {
  size_t i;

  printf("%lu", count);

  for (i = 0;  i < count;  ++i) {
    printf(" %lu", indices[i]);
  }

  putchar('\n'); 
}


void dump_triangles(const triangle_t *triangles, size_t triangle_count) {
  size_t t;
  size_t v;

  for (t = 0;  t < triangle_count;  ++t) {
    fputs("triangle:", stdout);
    for (v = 0;  v < 3;  ++v) {
      printf(" %lu", triangles[t].vertices[v]);
    }
    putchar('\n');
  }
}


int read_face(FILE *in, triangle_t *triangles, size_t *triangle_count, const vertex_t *vertices, size_t vertex_count) {
  char line[LINE_MAX];

  size_t indices[4];
  size_t index_count;

  if (read_line(in, line, LINE_MAX) == -1) return -1;

  if (extract_face(line, indices, &index_count) == -1) return -1;

  if (face_to_triangles(indices, index_count, triangles, triangle_count, vertices, vertex_count) == -1) return -1;

  return 0;
}


int write_vertex(FILE *out, const vertex_t *vertex) {
  if (fprintf(out, "    <%f, %f, %f>", vertex->x, vertex->y, vertex->z) < 0) {
    perror("write_vertex()");
    return -1;
  }

  return 0;
}


int write_triangle(FILE *out, const triangle_t *triangle, const vertex_t *vertices, size_t vertex_count) {
  size_t i;
  const vertex_t *vertex;

  if (fputs("  triangle {\n", out) == EOF) {
    perror("writing triangle header");
    return -1;
  }

  for (i = 0;  i < 3;  ++i) {
    if (find_vertex(&vertex, triangle->vertices[i], vertices, vertex_count) == -1) return -1;
    if (write_vertex(out, vertex) == -1) return -1;

    if (i < 2) {
      if (fputc(',', out) == EOF) {
        perror("writing a comma");
        return -1;
      }
    }

    if (fputc('\n', out) == EOF) {
      perror("writing a newline");
      return -1;
    }
  }

  if (fputs("  }\n", out) == EOF) {
    perror("writing triangle footer");
    return -1;
  }

  return 0;
}


int write_triangles(FILE *out, const triangle_t *triangles, size_t triangle_count, const vertex_t *vertices, size_t vertex_count) {
  size_t i;

  for (i = 0;  i < triangle_count;  ++i) {
    if (write_triangle(out, &triangles[i], vertices, vertex_count) == -1) {
      return -1;
    }
  }

  return 0;
}


int translate_faces(FILE *in, FILE *out, const vertex_t *vertices, size_t vertex_count, size_t face_count) {
  triangle_t triangles[2];
  size_t triangle_count;
  size_t i;

  for (i = 0;  i < face_count;  ++i) {
    if (read_face(in, triangles, &triangle_count, vertices, vertex_count) == -1) {
      return -1;
    }

    if (write_triangles(out, triangles, triangle_count, vertices, vertex_count) == -1) {
      return -1;
    }
  }

  return 0;
}


int write_ini_file(FILE *in, FILE *out, const vertex_t *vertices, size_t vertex_count, size_t face_count, const char *mesh_name) {
  if (write_ini_header(out, mesh_name) == -1) {
    return -1;
  }

  if (translate_faces(in, out, vertices, vertex_count, face_count) == -1) {
    return -1;
  }

  if (write_ini_footer(out) == -1) {
    return -1;
  }

  return 0;
}


void dump_vertices(const vertex_t *list, size_t count) {
  size_t i;

  for (i = 0;  i < count;  ++i) {
    printf("<%f, %f, %f>\n", list[i].x, list[i].y, list[i].z);
  }
}


int translate_ply(FILE *in, FILE *out, const char *mesh_name) {
  header_t header;
  vertex_t *vertices = NULL;
  int retval = 0;

  if (read_header(in, &header) == -1) {
    goto bad;
  }

  if (!(vertices = calloc(header.vertex_count, sizeof(*vertices)))) {
    perror("calloc()");
    goto bad;
  }

  if (read_vertices(in, vertices, header.vertex_count) == -1) {
    goto bad;
  }

  if (write_ini_file(in, out, vertices, header.vertex_count, header.face_count, mesh_name) == -1) {
    goto bad;
  }

 cleanup:
  if (vertices) {
    free(vertices);
  }

  return retval;

 bad:
  retval = -1;
  goto cleanup;
}


int main(int argc, char **argv) {
  char *input_file;
  char *output_file;
  char *mesh_name;

  FILE *input;
  FILE *output;

  if (argc != 4) {
    fprintf(stderr, "Usage: %s <input_file> <output_file> <mesh_name>\n", argv[0]);
    exit(1);
  }

  input_file = argv[1];
  output_file = argv[2];
  mesh_name = argv[3];

  if (!(input = fopen(input_file, "r"))) {
    fprintf(stderr, "Can't read %s: %s\n", input_file, strerror(errno));
    exit(1);
  }

  if (!(output = fopen(output_file, "w"))) {
    fprintf(stderr, "Can't write %s: %s\n", output_file, strerror(errno));
    exit(1);
  }

  if (translate_ply(input, output, mesh_name) == -1) {
    exit(1);
  }

  fclose(output);
  fclose(input);

  return 0;
}
