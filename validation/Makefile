VALIDATION_PROBES := \
	27_gl_api_probe \
	28_gl_vertex_array_probe \
	29_gl_texture_upload_probe \
	30_gl_display_list_probe \
	31_gl_raster_primitive_probe \
	32_gl_lighting_probe \
	33_gl_fog_probe \
	34_gl_readpixels_probe \
	35_gl_mipmap_probe \
	36_gl_selection_probe \
	37_gl_drawpixels_probe \
	38_gl_accum_probe \
	39_gl_matrix_query_probe \
	40_gl_drawrange_probe \
	41_gl_transpose_matrix_probe \
	42_gl_packed_pixels_probe \
	43_gl_light_model_probe \
	44_gl_texenv_probe \
	45_gl_combine_probe \
	46_gl_multitexture_render_probe \
	47_gl_cubemap_probe \
	48_gl_cubemap_render_probe \
	49_gl_compressed_texture_probe \
	50_gl_compressed_render_probe \
	51_gl_texture3d_probe \
	52_gl_texture3d_render_probe \
	53_gl_texture_lod_probe \
	54_gl_raster_state_probe \
	55_gl_alpha_logic_probe \
	56_gl_pixel_transfer_probe \
	57_gl_depth_stencil_pixels_probe \
	58_gl_state_query_probe \
	59_gl_interleaved_state_probe \
	60_gl_stipple_buffer_probe \
	61_gl_feedback_probe \
	62_gl_feedback_pixel_probe \
	63_gl_select_feedback_clip_probe \
	64_gl_state_query_limits_probe \
	65_gl_pixelstore_stride_probe \
	66_gl_pixelstore_3d_probe \
	67_gl_copytex_probe \
	68_gl_evaluator_probe \
	69_gl_texgen_probe \
	70_gl_attrib_stack_probe \
	71_gl_clip_plane_probe \
	72_gl_texture1d_probe \
	73_gl_api_surface_wrappers_probe \
	74_gl_depth_range_probe \
	75_gl_depth_func_probe \
	76_gl_viewport_clip_probe \
	77_gl_get_surface_probe \
	78_gl_feedback_line_clip_probe \
	79_gl_feedback_polygon_clip_probe \
	80_gl_render_polygon_clip_probe \
	81_gl_render_line_clip_probe \
	82_gl_render_point_raster_clip_probe \
	83_gl_shade_model_probe \
	84_gl_clear_mask_scissor_probe \
	85_gl_native_clear_visual_probe \
	86_gl_projection_render_probe \
	87_gl_projection_clip_probe \
	88_gl_display_list_state_probe \
	89_gl_get_audit_probe \
	90_gl_api_completeness_probe \
	91_gl_begin_end_illegal_ops_probe \
	92_gl_stencil_ops_probe \
	93_gl_blend_cull_probe \
	94_gl_indexed_pixel_map_probe \
	95_gl_pixel_bounds_probe \
	96_gl_alpha_logic_deep_probe \
	97_gl_raster_edge_rules_probe \
	98_gl_light_spot_atten_probe \
	99_gl_two_sided_light_probe \
	100_gl_select_feedback_edge_probe \
	101_gl_matrix_stack_edge_probe \
	102_gl_state_query_conversion_probe \
	103_gl_clip_plane_edge_probe \
	104_gl_depth_state_interlock_probe \
	105_gl_alpha_logic_interlock_probe \
	106_gl_pixel_transfer_interlock_probe \
	107_gl_select_feedback_guardband_probe \
	108_gl_backend_state_cache_probe \
	109_gl_fast_readback_probe \
	110_gl_perf_counter_probe

TEXTURE_PROBES := \
	29_gl_texture_upload_probe \
	35_gl_mipmap_probe \
	42_gl_packed_pixels_probe \
	44_gl_texenv_probe \
	45_gl_combine_probe \
	46_gl_multitexture_render_probe \
	47_gl_cubemap_probe \
	48_gl_cubemap_render_probe \
	49_gl_compressed_texture_probe \
	50_gl_compressed_render_probe \
	51_gl_texture3d_probe \
	52_gl_texture3d_render_probe \
	53_gl_texture_lod_probe \
	65_gl_pixelstore_stride_probe \
	66_gl_pixelstore_3d_probe \
	67_gl_copytex_probe \
	69_gl_texgen_probe \
	72_gl_texture1d_probe \
	108_gl_backend_state_cache_probe \
	109_gl_fast_readback_probe \
	110_gl_perf_counter_probe

LIGHTING_PROBES := \
	32_gl_lighting_probe \
	43_gl_light_model_probe \
	83_gl_shade_model_probe \
	98_gl_light_spot_atten_probe \
	99_gl_two_sided_light_probe

RASTER_EDGE_PROBES := \
	31_gl_raster_primitive_probe \
	54_gl_raster_state_probe \
	76_gl_viewport_clip_probe \
	78_gl_feedback_line_clip_probe \
	79_gl_feedback_polygon_clip_probe \
	80_gl_render_polygon_clip_probe \
	81_gl_render_line_clip_probe \
	82_gl_render_point_raster_clip_probe \
	84_gl_clear_mask_scissor_probe \
	85_gl_native_clear_visual_probe \
	87_gl_projection_clip_probe \
	93_gl_blend_cull_probe \
	95_gl_pixel_bounds_probe \
	97_gl_raster_edge_rules_probe \
	103_gl_clip_plane_edge_probe \
	104_gl_depth_state_interlock_probe \
	105_gl_alpha_logic_interlock_probe \
	106_gl_pixel_transfer_interlock_probe \
	107_gl_select_feedback_guardband_probe \
	108_gl_backend_state_cache_probe \
	109_gl_fast_readback_probe \
	110_gl_perf_counter_probe

.PHONY: all autorun texture lighting raster clean print-validation-apps

all:
	@for probe in $(VALIDATION_PROBES); do $(MAKE) -C $$probe || exit $$?; done

autorun:
	$(MAKE) -C autorun_suite

texture:
	@for probe in $(TEXTURE_PROBES); do $(MAKE) -C $$probe || exit $$?; done

lighting:
	@for probe in $(LIGHTING_PROBES); do $(MAKE) -C $$probe || exit $$?; done

raster:
	@for probe in $(RASTER_EDGE_PROBES); do $(MAKE) -C $$probe || exit $$?; done

clean:
	@for probe in $(VALIDATION_PROBES) autorun_suite; do $(MAKE) -C $$probe clean || exit $$?; done

print-validation-apps:
	@printf '%s\n' $(VALIDATION_PROBES)
