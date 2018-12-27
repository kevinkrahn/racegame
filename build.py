#!/usr/bin/python3

import os, sys, subprocess, urllib.request, zipfile, tempfile, errno, pickle, glob, time, shutil
from argparse import ArgumentParser

repoPath = subprocess.check_output(['git', 'rev-parse', '--show-toplevel']).decode('utf-8').strip()

def ensureDir(dir):
    try:
        os.makedirs(dir)
    except OSError as e:
        if e.errno != errno.EEXIST:
            raise

def fetch(url, name):
    resultPath = os.path.join('external', name)
    ensureDir('external')

    if '.zip' in url:
        if os.path.isdir(resultPath):
            return False
        else:
            tmpPath = tempfile.mktemp()
            urllib.request.urlretrieve(url, tmpPath)
            ensureDir(resultPath)
            zipfile.ZipFile(tmpPath, 'r').extractall(resultPath)
            return True
    else:
        if os.path.isdir(resultPath):
            #subprocess.run(['git', 'pull'], cwd=resultPath)
            return False
        else:
            subprocess.run(['git', 'clone', url, resultPath])
            return True

def fetch_dependencies():
    # http://glad.dav1d.de/#profile=core&api=gl%3D4.5&api=gles1%3Dnone&api=gles2%3Dnone&api=glsc2%3Dnone&extensions=GL_ARB_ES2_compatibility&extensions=GL_ARB_ES3_1_compatibility&extensions=GL_ARB_ES3_2_compatibility&extensions=GL_ARB_ES3_compatibility&extensions=GL_ARB_arrays_of_arrays&extensions=GL_ARB_base_instance&extensions=GL_ARB_bindless_texture&extensions=GL_ARB_blend_func_extended&extensions=GL_ARB_buffer_storage&extensions=GL_ARB_cl_event&extensions=GL_ARB_clear_buffer_object&extensions=GL_ARB_clear_texture&extensions=GL_ARB_clip_control&extensions=GL_ARB_color_buffer_float&extensions=GL_ARB_compatibility&extensions=GL_ARB_compressed_texture_pixel_storage&extensions=GL_ARB_compute_shader&extensions=GL_ARB_compute_variable_group_size&extensions=GL_ARB_conditional_render_inverted&extensions=GL_ARB_conservative_depth&extensions=GL_ARB_copy_buffer&extensions=GL_ARB_copy_image&extensions=GL_ARB_cull_distance&extensions=GL_ARB_debug_output&extensions=GL_ARB_depth_buffer_float&extensions=GL_ARB_depth_clamp&extensions=GL_ARB_depth_texture&extensions=GL_ARB_derivative_control&extensions=GL_ARB_direct_state_access&extensions=GL_ARB_draw_buffers&extensions=GL_ARB_draw_buffers_blend&extensions=GL_ARB_draw_elements_base_vertex&extensions=GL_ARB_draw_indirect&extensions=GL_ARB_draw_instanced&extensions=GL_ARB_enhanced_layouts&extensions=GL_ARB_explicit_attrib_location&extensions=GL_ARB_explicit_uniform_location&extensions=GL_ARB_fragment_coord_conventions&extensions=GL_ARB_fragment_layer_viewport&extensions=GL_ARB_fragment_program&extensions=GL_ARB_fragment_program_shadow&extensions=GL_ARB_fragment_shader&extensions=GL_ARB_fragment_shader_interlock&extensions=GL_ARB_framebuffer_no_attachments&extensions=GL_ARB_framebuffer_object&extensions=GL_ARB_framebuffer_sRGB&extensions=GL_ARB_geometry_shader4&extensions=GL_ARB_get_program_binary&extensions=GL_ARB_get_texture_sub_image&extensions=GL_ARB_gl_spirv&extensions=GL_ARB_gpu_shader5&extensions=GL_ARB_gpu_shader_fp64&extensions=GL_ARB_gpu_shader_int64&extensions=GL_ARB_half_float_pixel&extensions=GL_ARB_half_float_vertex&extensions=GL_ARB_imaging&extensions=GL_ARB_indirect_parameters&extensions=GL_ARB_instanced_arrays&extensions=GL_ARB_internalformat_query&extensions=GL_ARB_internalformat_query2&extensions=GL_ARB_invalidate_subdata&extensions=GL_ARB_map_buffer_alignment&extensions=GL_ARB_map_buffer_range&extensions=GL_ARB_matrix_palette&extensions=GL_ARB_multi_bind&extensions=GL_ARB_multi_draw_indirect&extensions=GL_ARB_multisample&extensions=GL_ARB_multitexture&extensions=GL_ARB_occlusion_query&extensions=GL_ARB_occlusion_query2&extensions=GL_ARB_parallel_shader_compile&extensions=GL_ARB_pipeline_statistics_query&extensions=GL_ARB_pixel_buffer_object&extensions=GL_ARB_point_parameters&extensions=GL_ARB_point_sprite&extensions=GL_ARB_polygon_offset_clamp&extensions=GL_ARB_post_depth_coverage&extensions=GL_ARB_program_interface_query&extensions=GL_ARB_provoking_vertex&extensions=GL_ARB_query_buffer_object&extensions=GL_ARB_robust_buffer_access_behavior&extensions=GL_ARB_robustness&extensions=GL_ARB_robustness_isolation&extensions=GL_ARB_sample_locations&extensions=GL_ARB_sample_shading&extensions=GL_ARB_sampler_objects&extensions=GL_ARB_seamless_cube_map&extensions=GL_ARB_seamless_cubemap_per_texture&extensions=GL_ARB_separate_shader_objects&extensions=GL_ARB_shader_atomic_counter_ops&extensions=GL_ARB_shader_atomic_counters&extensions=GL_ARB_shader_ballot&extensions=GL_ARB_shader_bit_encoding&extensions=GL_ARB_shader_clock&extensions=GL_ARB_shader_draw_parameters&extensions=GL_ARB_shader_group_vote&extensions=GL_ARB_shader_image_load_store&extensions=GL_ARB_shader_image_size&extensions=GL_ARB_shader_objects&extensions=GL_ARB_shader_precision&extensions=GL_ARB_shader_stencil_export&extensions=GL_ARB_shader_storage_buffer_object&extensions=GL_ARB_shader_subroutine&extensions=GL_ARB_shader_texture_image_samples&extensions=GL_ARB_shader_texture_lod&extensions=GL_ARB_shader_viewport_layer_array&extensions=GL_ARB_shading_language_100&extensions=GL_ARB_shading_language_420pack&extensions=GL_ARB_shading_language_include&extensions=GL_ARB_shading_language_packing&extensions=GL_ARB_shadow&extensions=GL_ARB_shadow_ambient&extensions=GL_ARB_sparse_buffer&extensions=GL_ARB_sparse_texture&extensions=GL_ARB_sparse_texture2&extensions=GL_ARB_sparse_texture_clamp&extensions=GL_ARB_spirv_extensions&extensions=GL_ARB_stencil_texturing&extensions=GL_ARB_sync&extensions=GL_ARB_tessellation_shader&extensions=GL_ARB_texture_barrier&extensions=GL_ARB_texture_border_clamp&extensions=GL_ARB_texture_buffer_object&extensions=GL_ARB_texture_buffer_object_rgb32&extensions=GL_ARB_texture_buffer_range&extensions=GL_ARB_texture_compression&extensions=GL_ARB_texture_compression_bptc&extensions=GL_ARB_texture_compression_rgtc&extensions=GL_ARB_texture_cube_map&extensions=GL_ARB_texture_cube_map_array&extensions=GL_ARB_texture_env_add&extensions=GL_ARB_texture_env_combine&extensions=GL_ARB_texture_env_crossbar&extensions=GL_ARB_texture_env_dot3&extensions=GL_ARB_texture_filter_anisotropic&extensions=GL_ARB_texture_filter_minmax&extensions=GL_ARB_texture_float&extensions=GL_ARB_texture_gather&extensions=GL_ARB_texture_mirror_clamp_to_edge&extensions=GL_ARB_texture_mirrored_repeat&extensions=GL_ARB_texture_multisample&extensions=GL_ARB_texture_non_power_of_two&extensions=GL_ARB_texture_query_levels&extensions=GL_ARB_texture_query_lod&extensions=GL_ARB_texture_rectangle&extensions=GL_ARB_texture_rg&extensions=GL_ARB_texture_rgb10_a2ui&extensions=GL_ARB_texture_stencil8&extensions=GL_ARB_texture_storage&extensions=GL_ARB_texture_storage_multisample&extensions=GL_ARB_texture_swizzle&extensions=GL_ARB_texture_view&extensions=GL_ARB_timer_query&extensions=GL_ARB_transform_feedback2&extensions=GL_ARB_transform_feedback3&extensions=GL_ARB_transform_feedback_instanced&extensions=GL_ARB_transform_feedback_overflow_query&extensions=GL_ARB_transpose_matrix&extensions=GL_ARB_uniform_buffer_object&extensions=GL_ARB_vertex_array_bgra&extensions=GL_ARB_vertex_array_object&extensions=GL_ARB_vertex_attrib_64bit&extensions=GL_ARB_vertex_attrib_binding&extensions=GL_ARB_vertex_blend&extensions=GL_ARB_vertex_buffer_object&extensions=GL_ARB_vertex_program&extensions=GL_ARB_vertex_shader&extensions=GL_ARB_vertex_type_10f_11f_11f_rev&extensions=GL_ARB_vertex_type_2_10_10_10_rev&extensions=GL_ARB_viewport_array&extensions=GL_ARB_window_pos&extensions=GL_EXT_422_pixels&extensions=GL_EXT_EGL_image_storage&extensions=GL_EXT_abgr&extensions=GL_EXT_bgra&extensions=GL_EXT_bindable_uniform&extensions=GL_EXT_blend_color&extensions=GL_EXT_blend_equation_separate&extensions=GL_EXT_blend_func_separate&extensions=GL_EXT_blend_logic_op&extensions=GL_EXT_blend_minmax&extensions=GL_EXT_blend_subtract&extensions=GL_EXT_clip_volume_hint&extensions=GL_EXT_cmyka&extensions=GL_EXT_color_subtable&extensions=GL_EXT_compiled_vertex_array&extensions=GL_EXT_convolution&extensions=GL_EXT_coordinate_frame&extensions=GL_EXT_copy_texture&extensions=GL_EXT_cull_vertex&extensions=GL_EXT_debug_label&extensions=GL_EXT_debug_marker&extensions=GL_EXT_depth_bounds_test&extensions=GL_EXT_direct_state_access&extensions=GL_EXT_draw_buffers2&extensions=GL_EXT_draw_instanced&extensions=GL_EXT_draw_range_elements&extensions=GL_EXT_external_buffer&extensions=GL_EXT_fog_coord&extensions=GL_EXT_framebuffer_blit&extensions=GL_EXT_framebuffer_multisample&extensions=GL_EXT_framebuffer_multisample_blit_scaled&extensions=GL_EXT_framebuffer_object&extensions=GL_EXT_framebuffer_sRGB&extensions=GL_EXT_geometry_shader4&extensions=GL_EXT_gpu_program_parameters&extensions=GL_EXT_gpu_shader4&extensions=GL_EXT_histogram&extensions=GL_EXT_index_array_formats&extensions=GL_EXT_index_func&extensions=GL_EXT_index_material&extensions=GL_EXT_index_texture&extensions=GL_EXT_light_texture&extensions=GL_EXT_memory_object&extensions=GL_EXT_memory_object_fd&extensions=GL_EXT_memory_object_win32&extensions=GL_EXT_misc_attribute&extensions=GL_EXT_multi_draw_arrays&extensions=GL_EXT_multisample&extensions=GL_EXT_packed_depth_stencil&extensions=GL_EXT_packed_float&extensions=GL_EXT_packed_pixels&extensions=GL_EXT_paletted_texture&extensions=GL_EXT_pixel_buffer_object&extensions=GL_EXT_pixel_transform&extensions=GL_EXT_pixel_transform_color_table&extensions=GL_EXT_point_parameters&extensions=GL_EXT_polygon_offset&extensions=GL_EXT_polygon_offset_clamp&extensions=GL_EXT_post_depth_coverage&extensions=GL_EXT_provoking_vertex&extensions=GL_EXT_raster_multisample&extensions=GL_EXT_rescale_normal&extensions=GL_EXT_secondary_color&extensions=GL_EXT_semaphore&extensions=GL_EXT_semaphore_fd&extensions=GL_EXT_semaphore_win32&extensions=GL_EXT_separate_shader_objects&extensions=GL_EXT_separate_specular_color&extensions=GL_EXT_shader_framebuffer_fetch&extensions=GL_EXT_shader_framebuffer_fetch_non_coherent&extensions=GL_EXT_shader_image_load_formatted&extensions=GL_EXT_shader_image_load_store&extensions=GL_EXT_shader_integer_mix&extensions=GL_EXT_shadow_funcs&extensions=GL_EXT_shared_texture_palette&extensions=GL_EXT_sparse_texture2&extensions=GL_EXT_stencil_clear_tag&extensions=GL_EXT_stencil_two_side&extensions=GL_EXT_stencil_wrap&extensions=GL_EXT_subtexture&extensions=GL_EXT_texture&extensions=GL_EXT_texture3D&extensions=GL_EXT_texture_array&extensions=GL_EXT_texture_buffer_object&extensions=GL_EXT_texture_compression_latc&extensions=GL_EXT_texture_compression_rgtc&extensions=GL_EXT_texture_compression_s3tc&extensions=GL_EXT_texture_cube_map&extensions=GL_EXT_texture_env_add&extensions=GL_EXT_texture_env_combine&extensions=GL_EXT_texture_env_dot3&extensions=GL_EXT_texture_filter_anisotropic&extensions=GL_EXT_texture_filter_minmax&extensions=GL_EXT_texture_integer&extensions=GL_EXT_texture_lod_bias&extensions=GL_EXT_texture_mirror_clamp&extensions=GL_EXT_texture_object&extensions=GL_EXT_texture_perturb_normal&extensions=GL_EXT_texture_sRGB&extensions=GL_EXT_texture_sRGB_R8&extensions=GL_EXT_texture_sRGB_decode&extensions=GL_EXT_texture_shared_exponent&extensions=GL_EXT_texture_snorm&extensions=GL_EXT_texture_swizzle&extensions=GL_EXT_timer_query&extensions=GL_EXT_transform_feedback&extensions=GL_EXT_vertex_array&extensions=GL_EXT_vertex_array_bgra&extensions=GL_EXT_vertex_attrib_64bit&extensions=GL_EXT_vertex_shader&extensions=GL_EXT_vertex_weighting&extensions=GL_EXT_win32_keyed_mutex&extensions=GL_EXT_window_rectangles&extensions=GL_EXT_x11_sync_object&extensions=GL_KHR_context_flush_control&extensions=GL_KHR_debug&extensions=GL_KHR_no_error&extensions=GL_KHR_parallel_shader_compile&extensions=GL_KHR_robust_buffer_access_behavior&extensions=GL_KHR_robustness&extensions=GL_KHR_texture_compression_astc_hdr&extensions=GL_KHR_texture_compression_astc_ldr&extensions=GL_KHR_texture_compression_astc_sliced_3d&language=c&specification=gl&localfiles=on
    fetch('https://github.com/nothings/stb.git', 'stb')
    fetch('https://github.com/g-truc/glm', 'glm')

    if fetch('https://github.com/NVIDIAGameWorks/PhysX', 'physx'):
        physxdir = os.path.join('external', 'physx', 'physx')
        if sys.platform == 'win32':
            subprocess.run([os.path.join(physxDir, 'generate_projects.bat'), 'win32'], cwd=physxdir)
            # TODO
        else:
            subprocess.run([os.path.join(physxdir, 'generate_projects.sh'), 'linux'], cwd=physxdir)
            subprocess.run(['cmake', '--build', os.path.join(physxdir, 'compiler', 'linux-checked'),
                '--parallel', str(os.cpu_count())], cwd=physxdir)
            subprocess.run(['cmake', '--build', os.path.join(physxdir, 'compiler', 'linux-release'),
                '--parallel', str(os.cpu_count())], cwd=physxdir)

    if sys.platform == 'win32':
        if fetch('https://github.com/spurious/SDL-mirror', 'sdl2'):
            # TODO
            pass


def build_assets(force):
    timestampCacheFilename = os.path.join('build', 'asset_timestamps')
    timestampCache = {}

    if not force:
        try:
            with open(timestampCacheFilename, 'rb') as file:
                timestampCache = pickle.load(file)
        except OSError as e:
            if e.errno != errno.ENOENT:
                raise

    def exportFile(blendFile):
        blenderCommand = 'blender' if sys.platform != 'win32' else '"C:/Program Files/Blender Foundation/Blender/blender.exe"'
        output = subprocess.check_output([blenderCommand, '-b', blendFile, '-P', 'blender_exporter.py', '--enable-autoexec']).decode('utf-8')
        return 'Saved to file:' in output

    def copyFile(file):
        dest = os.path.join('bin', os.path.relpath(file, 'assets'))
        ensureDir(os.path.dirname(dest))
        shutil.copy2(file, dest)

    modified = False
    for file in glob.glob('assets/**/*', recursive=True):
        if not os.path.isfile(file):
            continue

        timestamp = os.path.getmtime(file)
        #print(file, ':', timestamp)
        if timestampCache.get(file, 0) < timestamp:
            if file.endswith(".blend"):
                print('Exporting', file)
                if exportFile(file):
                    modified = True
                    timestampCache[file] = timestamp
            else:
                print('Copying', file)
                copyFile(file)
                modified = True
                timestampCache[file] = timestamp

    if modified:
        ensureDir('build')
        with open(timestampCacheFilename, 'wb') as file:
            pickle.dump(timestampCache, file)
    else:
        print('No assets to save')

def build():
    build_type = 'debug'
    job_count = str(os.cpu_count())

    ensureDir('bin')

    returncode = False
    if sys.platform == 'win32':
        build_dir = 'build'
        if not os.path.isdir(build_dir):
            ensureDir(build_dir)
            if subprocess.run(['cmake', repoPath], cwd=build_dir).returncode != 0:
                return False
        if subprocess.run(['cmake', '--build', '--target', build_type, '--parallel', job_count]).returncode != 0:
            return False
    else:
        build_dir = os.path.join('build', build_type)
        if not os.path.isdir(build_dir):
            ensureDir(build_dir)
            if subprocess.run(['cmake', repoPath, '-DCMAKE_BUILD_TYPE='+build_type, '-DCMAKE_C_COMPILER=clang', '-DCMAKE_CXX_COMPILER=clang++'], cwd=build_dir).returncode != 0:
                return False
        if subprocess.run(['cmake', '--build', build_dir, '--parallel', job_count]).returncode != 0:
            return False
    return True

def cleanBuild():
    shutil.rmtree('build')
    shutil.rmtree('bin')

parser = ArgumentParser(description='This is a build tool.')
subparsers = parser.add_subparsers(dest='command')
compile = subparsers.add_parser('compile')
clean = subparsers.add_parser('clean')
assets = subparsers.add_parser('assets')
deps = subparsers.add_parser('deps')

args = parser.parse_args()

os.chdir(repoPath)

start_time = time.time()

if args.command == 'compile':
    build()
elif args.command == 'clean':
    cleanBuild()
elif args.command == 'assets':
    build_assets(False)
elif args.command == 'deps':
    fetch_dependencies()
else:
    fetch_dependencies()
    build_assets(False)
    if build():
        subprocess.run([os.path.abspath(os.path.join('bin', 'game'))], cwd=os.path.abspath('bin'))

print('Took {:.2f} seconds'.format(time.time() - start_time))
