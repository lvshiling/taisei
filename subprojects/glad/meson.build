project('glad', 'c')

glad_profile = 'core'
glad_api = ['gl=3.3', 'gles2=3.0']
glad_extensions = [
    'GL_ANGLE_depth_texture',
    'GL_ANGLE_instanced_arrays',
    'GL_ANGLE_translated_shader_source',
    'GL_ARB_base_instance',
    'GL_ARB_clear_texture',
    'GL_ARB_debug_output',
    'GL_ARB_depth_texture',
    'GL_ARB_draw_buffers',
    'GL_ARB_draw_instanced',
    'GL_ARB_instanced_arrays',
    'GL_ARB_pixel_buffer_object',
    'GL_ARB_texture_filter_anisotropic',
    'GL_ARB_vertex_array_object',
    'GL_ARB_viewport_array',
    'GL_ATI_draw_buffers',
    'GL_EXT_base_instance',
    'GL_EXT_clear_texture',
    'GL_EXT_color_buffer_float',
    'GL_EXT_draw_buffers',
    'GL_EXT_draw_instanced',
    'GL_EXT_float_blend',
    'GL_EXT_instanced_arrays',
    'GL_EXT_pixel_buffer_object',
    'GL_EXT_texture_filter_anisotropic',
    'GL_EXT_texture_norm16',
    'GL_EXT_texture_rg',
    'GL_KHR_debug',
    'GL_NV_draw_instanced',
    'GL_NV_instanced_arrays',
    'GL_NV_pixel_buffer_object',
    'GL_NV_viewport_array2',
    'GL_OES_depth_texture',
    'GL_OES_texture_float_linear',
    'GL_OES_texture_half_float_linear',
    'GL_OES_vertex_array_object',
    'GL_OES_viewport_array',
    'GL_SGIX_depth_texture',
]

# NOTE: requires glad2 (beta) https://github.com/Dav1dde/glad/tree/glad2
glad_program = find_program('glad', required : false)

glad_args = [
    '--api', 'gl:core=3.3,gles2=3.0',
    '--extensions', ','.join(glad_extensions),
    '--merge',
    '--out-path', meson.current_source_dir(),
    'c',
    '--alias',
]

glad_sources = []
glad_includes = include_directories('include')

subdir('src')

glad_lib = static_library('glad', glad_sources,
    install : not meson.is_subproject(),
    build_by_default : not meson.is_subproject(),
    include_directories : glad_includes,
)

glad_dep = declare_dependency(
  include_directories : glad_includes,
  link_with : glad_lib
)

if glad_program.found()
    run_target('gen-gl-loader',
        command : [glad_program, glad_args],
    )
endif
