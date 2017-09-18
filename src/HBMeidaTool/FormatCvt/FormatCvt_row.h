#ifndef _H_FORMATCVT_ROW_H
#define _H_FORMATCVT_ROW_H
			 void J422ToARGBRow_C(const uint8* src_y,
	                     const uint8* src_u,
	                     const uint8* src_v,
	                     uint8* dst_argb,
	                     int width);

			 void J422ToARGBRow_NEON(const uint8* src_y,
	                        		const uint8* src_u,
	                        		const uint8* src_v,
	                        		uint8* dst_argb,
	                       			int width);


			 void J422ToARGBRow_Any_NEON(const uint8* y_buf,
	                                    const uint8* u_buf, 
	                                    const uint8* v_buf, 
	                                    uint8* rgb_buf, int width);


			 void ARGBToUVJ422Row_C(const uint8* src_argb,
	                       			uint8* dst_u, 
	                       			uint8* dst_v, 
	                       			int width);

			 void ARGBToYJRow_C(const uint8* src_argb, 
									uint8* dst_y,
									 int pix);

			 void ARGBToUVJRow_C(const uint8* src_rgb0, int src_stride_rgb,             
	                        uint8* dst_u, uint8* dst_v, int width);


			 void ARGBToUVJ422Row_NEON(const uint8* src_argb,
										uint8* dst_u, 
										uint8* dst_v, 
										int pix);

			 void ARGBToYJRow_NEON(const uint8* src_argb,
										 uint8* dst_y, 
										 int pix);

			 void ARGBToUVJRow_NEON(const uint8* src_argb,
									int src_stride_argb,
	                       			uint8* dst_u, 
	                       			uint8* dst_v, 
	                       			int pix);
		
			 void YUV444ToYUY2Row_C(const uint8* src_yuv444,uint8* dst_yuy2,int width);
			 void YUV444ToYUY2Row_NEON(const uint8* src_yuy2,uint8* dst_yuv444,int pix);
			 void YUV444ToYUY2Row_Any_NEON(const uint8* src_argb, uint8* dst_y, int width);
			 //---------------------------------------------------------------------------
			 void YUY2ToYUV444Row_NEON(const uint8* src_yuy2,uint8* dst_yuv444,int width);
			 void YUY2ToYUV444Row_C(const uint8* src_yuy2,uint8* dst_yuv444,int width);
			 void YUY2ToYUV444Row_Any_NEON(const uint8* src_argb, uint8* dst_y, int width);
			 //---------------------------------------------------------------------------

			 void ARGBToUVJ422Row_Any_NEON(const uint8* src_argb,
	                          			 uint8* dst_u, 
	                          			 uint8* dst_v, 
	                          			 int pix);

			 void ARGBToUVJRow_Any_NEON(const uint8* src_argb, 
										int src_stride_argb,
			                           uint8* dst_u, 
			                           uint8* dst_v, 
			                           int pix);

			 void ARGBToYJRow_Any_NEON(const uint8* src_argb, 
										uint8* dst_y, 
										int pix);

			 void ARGBToUVRow_NEON(const uint8* src_argb, int src_stride_argb,
	                      uint8* dst_u, uint8* dst_v, int pix) ;

			 void ARGBToYRow_NEON(const uint8* src_argb, uint8* dst_y, int pix);

			 void ARGBToUV422Row_NEON(const uint8* src_argb, 
											uint8* dst_u, 
											uint8* dst_v,
								            int pix);

			 void I422ToARGBRow_NEON(const uint8* src_y,
					                        const uint8* src_u,
					                        const uint8* src_v,
					                        uint8* dst_argb,
					                        int width);

			 void ARGBToUV422Row_C(const uint8* src_argb,
					                      uint8* dst_u,
					                      uint8* dst_v,
					                      int width);

			 void ARGBToYRow_C(const uint8* src_argb, 
									uint8* dst_y, 
									int pix);

			 void ARGBToUVRow_C(const uint8* src_argb, 
										int src_stride_argb,
					                   uint8* dst_u, 
					                   uint8* dst_v, 
					                   int width);

			 void I422ToARGBRow_C(const uint8* src_y,
					                     const uint8* src_u,
					                     const uint8* src_v,
					                     uint8* rgb_buf,
					                     int width);

			 void ARGBToUVRow_Any_NEON(const uint8* src_argb, 
											  int src_stride_argb,
					                          uint8* dst_u, 
					                          uint8* dst_v,
					                           int pix);

			 void ARGBToYRow_Any_NEON(const uint8* src_argb, 
											uint8* dst_y, 
											int pix);

			 void ARGBToUV422Row_Any_NEON(const uint8* src_argb, 
										uint8* dst_u, 
										uint8* dst_v,
							            int pix);

			 void I422ToARGBRow_Any_NEON(const uint8* src_y,
			                            const uint8* src_u,
			                            const uint8* src_v,
			                            uint8* dst_argb,
			                            int width);


			 void MergeUVRow_C(const uint8* src_u,
								const uint8* src_v,
								uint8* dst_uv,
				                 int width);

			 void MergeUVRow_NEON(const uint8* src_u, 
									const uint8* src_v, 
									uint8* dst_uv,
					                int width);

			 void MergeUVRow_Any_NEON(const uint8* src_u, 
										const uint8* src_v, 
										uint8* dst_uv,
						                int width);

			 void NV12ToARGBRow_C(const uint8* src_y,
			                     const uint8* usrc_v,
			                     uint8* rgb_buf,
			                     int width);

			 void NV12ToARGBRow_NEON(const uint8* src_y,
			                        const uint8* src_uv,
			                        uint8* dst_argb,
			                        int width);

			 void NV12ToARGBRow_Any_NEON(const uint8* src_y,
			                            const uint8* src_uv,
			                            uint8* dst_argb,
			                            int width);

			 void NV21ToARGBRow_C(const uint8* src_y,
                     const uint8* src_vu,
                     uint8* dst_argb,
                     int width);

			 void NV21ToARGBRow_NEON(const uint8* src_y,
                        const uint8* src_uv,
                        uint8* dst_argb,
                        int width);

			 void NV21ToARGBRow_Any_NEON(const uint8* src_y,
                            const uint8* src_uv,
                            uint8* dst_argb,
                            int width);
			 
			//----------------------------------
			void YUY2JToARGBRow_C(const uint8* src_yuy2,
                     uint8* rgb_buf,
                     int width);

			void YUY2JToARGBRow_NEON(const uint8* src_yuy2,
                        uint8* dst_argb,
                        int width);

			void YUY2JToARGBRow_Any_NEON(const uint8* src_yuy2,
                            uint8* dst_argb,
                            int width);

			void I422ToYUY2JRow_C(const uint8* src_y,
                     const uint8* src_u,
                     const uint8* src_v,
                     uint8* dst_frame, int width);

			void I422ToYUY2JRow_NEON(const uint8* src_y,
                        const uint8* src_u,
                        const uint8* src_v,
                        uint8* dst_yuy2, int width);

			void I422ToYUY2JRow_Any_NEON(const uint8* src_y,
                            const uint8* src_u,
                            const uint8* src_v,
                            uint8* dst_yuy2, int width);
#endif