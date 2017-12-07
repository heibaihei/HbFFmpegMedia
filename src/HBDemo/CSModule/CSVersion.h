//
//  CSVersion.h
//  Sample
//
//  Created by zj-db0519 on 2017/12/7.
//  Copyright © 2017年 meitu. All rights reserved.
//

#ifndef CSVersion_h
#define CSVersion_h

/** CSModule 版本信息：
 *    {CS_MODULE_NAME}-{CS_MAIN_VERSION}.{CS_RELEASE_VERSION}.{CS_UPGRADE_VERSION}.{CS_ALPHA_VERSION} - {CS_BETA_VERSION}
 *    例如： 0.6.8.2.0-(Beta:0)
 */
/** 底层库模块名 */
#define CS_MODULE_NAME         "CSMedia"

/** 主版本号，大范围修改才改动该版本号 */
#define CS_MAIN_VERSION        0.1

/** Release 版本号，不同的 release 版本，调整该值 */
#define CS_RELEASE_VERSION     0

/** 版本更新 或者 较大范围的改动, 主要应用场景如下：
 *     release 分支Bug修复、大的功能性开发，调整该版本号
 */
#define CS_UPGRADE_VERSION     0

/** 内部测试版，小范围的改动调整，使用该版本号;
 *  发布 release 版本后，当前release 分支的 MT_ALPHA_VERSION 置 0
 */
#define CS_ALPHA_VERSION       1

/** 外部测试版，提供零时分支时，启用该版本号 */
#define CS_BETA_VERSION        0

#endif /* CSVersion_h */
