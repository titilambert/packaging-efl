#include "shader/evas_gl_shaders.x"

/////////////////////////////////////////////
static void
gl_compile_link_error(GLuint target, const char *action)
{
   int loglen = 0, chars = 0;
   char *logtxt;

   /* Shader info log */
   glGetShaderiv(target, GL_INFO_LOG_LENGTH, &loglen);
   if (loglen > 0)
     {
        logtxt = calloc(loglen, sizeof(char));
        if (logtxt)
          {
             glGetShaderInfoLog(target, loglen, &chars, logtxt);
             ERR("Failed to %s: %s", action, logtxt);
             free(logtxt);
          }
     }

   /* Program info log */
   glGetProgramiv(target, GL_INFO_LOG_LENGTH, &loglen);
   if (loglen > 0)
     {
        logtxt = calloc(loglen, sizeof(char));
        if (logtxt)
          {
             glGetProgramInfoLog(target, loglen, &chars, logtxt);
             ERR("Failed to %s: %s", action, logtxt);
             free(logtxt);
          }
     }
}

static int
_evas_gl_common_shader_program_binary_init(Evas_GL_Program *p,
                                           const char *pname,
                                           Eet_File *ef)
{
   int res = 0, num = 0, length = 0;
   int *formats = NULL;
   void *data = NULL;
   GLint ok = 0;

   if (!ef) return res;

   data = eet_read(ef, pname, &length);
   if ((!data) || (length <= 0)) goto finish;

   glGetIntegerv(GL_NUM_PROGRAM_BINARY_FORMATS, &num);
   if (num <= 0) goto finish;

   formats = calloc(num, sizeof(int));
   if (!formats) goto finish;

   glGetIntegerv(GL_PROGRAM_BINARY_FORMATS, formats);
   if (!formats[0]) goto finish;

   p->prog = glCreateProgram();

#if 1
   // TODO: invalid rendering error occurs when attempting to use a
   // glProgramBinary. in order to render correctly we should create a dummy
   // vertex shader.
   p->vert = glCreateShader(GL_VERTEX_SHADER);
   glAttachShader(p->prog, p->vert);
   p->frag = glCreateShader(GL_FRAGMENT_SHADER);
   glAttachShader(p->prog, p->frag);
#endif
   glsym_glProgramBinary(p->prog, formats[0], data, length);
   GLERRV("glsym_glProgramBinary");

   glBindAttribLocation(p->prog, SHAD_VERTEX, "vertex");
   glBindAttribLocation(p->prog, SHAD_COLOR,  "color");
   glBindAttribLocation(p->prog, SHAD_TEXUV,  "tex_coord");
   glBindAttribLocation(p->prog, SHAD_TEXUV2, "tex_coord2");
   glBindAttribLocation(p->prog, SHAD_TEXUV3, "tex_coord3");
   glBindAttribLocation(p->prog, SHAD_TEXA,   "tex_coorda");
   glBindAttribLocation(p->prog, SHAD_TEXSAM, "tex_sample");
   glBindAttribLocation(p->prog, SHAD_MASK,   "mask_coord");

   glGetProgramiv(p->prog, GL_LINK_STATUS, &ok);
   if (!ok)
     {
        gl_compile_link_error(p->prog, "load a program object");
        ERR("Abort load of program (%s)", pname);
        goto finish;
     }

   res = 1;

finish:
   if (formats) free(formats);
   if (data) free(data);
   if ((!res) && (p->prog))
     {
        glDeleteProgram(p->prog);
        p->prog = 0;
     }
   return res;
}

static int
_evas_gl_common_shader_program_binary_save(Evas_GL_Program *p,
                                           const char *pname,
                                           Eet_File *ef)
{
   void* data = NULL;
   GLenum format;
   int length = 0, size = 0;

   if (!glsym_glGetProgramBinary) return 0;

   glGetProgramiv(p->prog, GL_PROGRAM_BINARY_LENGTH, &length);
   if (length <= 0) return 0;

   data = malloc(length);
   if (!data) return 0;

   glsym_glGetProgramBinary(p->prog, length, &size, &format, data);
   GLERRV("glsym_glGetProgramBinary");

   if (length != size)
     {
        free(data);
        return 0;
     }

   if (eet_write(ef, pname, data, length, 1) < 0)
     {
        if (data) free(data);
        return 0;
     }
   if (data) free(data);
   return 1;
}

static int
_evas_gl_common_shader_program_source_init(Evas_GL_Program *p,
                                           Evas_GL_Program_Source *vert,
                                           Evas_GL_Program_Source *frag,
                                           const char *name)
{
   GLint ok;

   p->vert = glCreateShader(GL_VERTEX_SHADER);
   p->frag = glCreateShader(GL_FRAGMENT_SHADER);

   glShaderSource(p->vert, 1,
                  (const char **)&(vert->src), NULL);
   glCompileShader(p->vert);
   ok = 0;
   glGetShaderiv(p->vert, GL_COMPILE_STATUS, &ok);
   if (!ok)
     {
        gl_compile_link_error(p->vert, "compile vertex shader");
        ERR("Abort compile of shader vert (%s): %s", name, vert->src);
        return 0;
     }
   glShaderSource(p->frag, 1,
                  (const char **)&(frag->src), NULL);
   glCompileShader(p->frag);
   ok = 0;
   glGetShaderiv(p->frag, GL_COMPILE_STATUS, &ok);
   if (!ok)
     {
        gl_compile_link_error(p->frag, "compile fragment shader");
        ERR("Abort compile of shader frag (%s): %s", name, frag->src);
        return 0;
     }

   p->prog = glCreateProgram();
#ifdef GL_GLES
#else
   if ((glsym_glGetProgramBinary) && (glsym_glProgramParameteri))
     {
        glsym_glProgramParameteri(p->prog, GL_PROGRAM_BINARY_RETRIEVABLE_HINT,
                                  GL_TRUE);
        GLERRV("glsym_glProgramParameteri");
     }
#endif
   glAttachShader(p->prog, p->vert);
   glAttachShader(p->prog, p->frag);

   glBindAttribLocation(p->prog, SHAD_VERTEX, "vertex");
   glBindAttribLocation(p->prog, SHAD_COLOR,  "color");
   glBindAttribLocation(p->prog, SHAD_TEXUV,  "tex_coord");
   glBindAttribLocation(p->prog, SHAD_TEXUV2, "tex_coord2");
   glBindAttribLocation(p->prog, SHAD_TEXUV3, "tex_coord3");
   glBindAttribLocation(p->prog, SHAD_TEXA,   "tex_coorda");
   glBindAttribLocation(p->prog, SHAD_TEXSAM, "tex_sample");
   glBindAttribLocation(p->prog, SHAD_MASK,   "mask_coord");

   glLinkProgram(p->prog);
   ok = 0;
   glGetProgramiv(p->prog, GL_LINK_STATUS, &ok);
   if (!ok)
     {
        gl_compile_link_error(p->prog, "link fragment and vertex shaders");
        ERR("Abort compile of shader frag (%s): %s", name, frag->src);
        ERR("Abort compile of shader vert (%s): %s", name, vert->src);
        return 0;
     }

   return 1;
}

static int
_evas_gl_common_shader_source_init(Evas_GL_Shared *shared)
{
  unsigned int i;

  for (i = 0; i < sizeof (_shaders_source) / sizeof (_shaders_source[0]); i++)
     {
        if (!_evas_gl_common_shader_program_source_init
            (&(shared->shader[_shaders_source[i].id]),
                _shaders_source[i].vert,
                _shaders_source[i].frag,
                _shaders_source[i].name))
          return 0;
     }
   return 1;
}

static int
_evas_gl_common_shader_binary_init(Evas_GL_Shared *shared)
{
   /* check eet */
   Eet_File *et = NULL;
   char bin_dir_path[PATH_MAX];
   char bin_file_path[PATH_MAX];
   unsigned int i;

   if (!evas_gl_common_file_cache_dir_check(bin_dir_path, sizeof(bin_dir_path)))
      return 0;

   if (!evas_gl_common_file_cache_file_check(bin_dir_path, "binary_shader", bin_file_path,
                                   sizeof(bin_dir_path)))
      return 0;

   /* use eet */
   if (!eet_init()) return 0;
   et = eet_open(bin_file_path, EET_FILE_MODE_READ);
   if (!et) goto error;

   for (i = 0; i < sizeof (_shaders_source) / sizeof (_shaders_source[0]); ++i)
     if (!_evas_gl_common_shader_program_binary_init(&(shared->shader[_shaders_source[i].id]),
                                                     _shaders_source[i].name,
                                                     et))
       goto error;

   if (et) eet_close(et);
   eet_shutdown();
   return 1;

error:
   if (et) eet_close(et);
   eet_shutdown();
   return 0;
}

static int
_evas_gl_common_shader_binary_save(Evas_GL_Shared *shared)
{
   /* check eet */
   Eet_File *et = NULL; //check eet file
   int tmpfd;
   int res = 0;
   char bin_dir_path[PATH_MAX];
   char bin_file_path[PATH_MAX];
   char tmp_file[PATH_MAX];
   unsigned int i;

   if (!evas_gl_common_file_cache_dir_check(bin_dir_path, sizeof(bin_dir_path)))
     {
        res = evas_gl_common_file_cache_mkpath(bin_dir_path);
        if (!res) return 0; /* we can't make directory */
     }

   evas_gl_common_file_cache_file_check(bin_dir_path, "binary_shader", bin_file_path,
                              sizeof(bin_dir_path));

   /* use mkstemp for writing */
   snprintf(tmp_file, sizeof(tmp_file), "%s.XXXXXX", bin_file_path);

#ifndef _WIN32
   mode_t old_umask = umask(S_IRWXG|S_IRWXO);
#endif
   tmpfd = mkstemp(tmp_file);
#ifndef _WIN32
   umask(old_umask);
#endif

   if (tmpfd < 0) goto error;
   close(tmpfd);

   /* use eet */
   if (!eet_init()) goto error;

   et = eet_open(tmp_file, EET_FILE_MODE_WRITE);
   if (!et) goto error;

   for (i = 0; i < sizeof (_shaders_source) / sizeof (_shaders_source[0]); ++i)
     if (!_evas_gl_common_shader_program_binary_save(&(shared->shader[_shaders_source[i].id]),
                                                     _shaders_source[i].name,
                                                     et))
       goto error;

   if (eet_close(et) != EET_ERROR_NONE) goto error;
   if (rename(tmp_file,bin_file_path) < 0) goto error;
   eet_shutdown();
   return 1;

error:
   if (et) eet_close(et);
   if (evas_gl_common_file_cache_file_exists(tmp_file)) unlink(tmp_file);
   eet_shutdown();
   return 0;
}

int
evas_gl_common_shader_program_init(Evas_GL_Shared *shared)
{
   // gl support binary shader and get env of binary shader path
   if (shared->info.bin_program &&
       _evas_gl_common_shader_binary_init(shared)) return 1;
   /* compile all shader.*/
   if (!_evas_gl_common_shader_source_init(shared)) return 0;
   /* success compile all shader. if gl support binary shader, we need to save */
   if (shared->info.bin_program) _evas_gl_common_shader_binary_save(shared);
   return 1;
}

void
evas_gl_common_shader_program_init_done(void)
{
#ifdef GL_GLES
   glReleaseShaderCompiler();
#else
   if (glsym_glReleaseShaderCompiler)
     {
        glsym_glReleaseShaderCompiler();
        GLERRV("glsym_glReleaseShaderCompiler");
     }
#endif
}

void
evas_gl_common_shader_program_shutdown(Evas_GL_Program *p)
{
   if (p->vert) glDeleteShader(p->vert);
   if (p->frag) glDeleteShader(p->frag);
   if (p->prog) glDeleteProgram(p->prog);
}

Evas_GL_Shader
evas_gl_common_img_shader_select(Shader_Sampling sam, int nomul, int afill, int bgra, int mask)
{
   static Evas_GL_Shader _shaders[4 * 2 * 2 * 2 * 2]; // 128 possibilities
   static Eina_Bool init = EINA_FALSE;
   int idx;

   if (EINA_UNLIKELY(!init))
     {
        unsigned k;

        init = EINA_TRUE;
        for (k = 0; k < (sizeof(_shaders) / sizeof(_shaders[0])); k++)
          _shaders[k] = SHADER_IMG;

        for (k = 0; k < (sizeof(_shaders_source) / sizeof(_shaders_source[0])); k++)
          {
             if (_shaders_source[k].type != SHD_IMAGE) continue;
             idx = _shaders_source[k].sam << 4;
             idx |= _shaders_source[k].bgra << 3;
             idx |= _shaders_source[k].mask << 2;
             idx |= _shaders_source[k].nomul << 1;
             idx |= _shaders_source[k].afill;
             _shaders[idx] = _shaders_source[k].id;
          }
     }

   idx = sam << 4;
   idx |= bgra << 3;
   idx |= mask << 2;
   idx |= nomul << 1;
   idx |= afill;
   return _shaders[idx];
}

const char *
evas_gl_common_shader_name_get(Evas_GL_Shader shd)
{
   if (shd < (sizeof(_shaders_source) / sizeof(_shaders_source[0])))
     return _shaders_source[shd].name;
   return "UNKNOWN";
}
