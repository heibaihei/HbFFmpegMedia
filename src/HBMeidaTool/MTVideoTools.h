class MTVideoTools{
public:
    MTVideoTools();
    ~MTVideoTools();
    /*进行快速的视频信息转移，
    * infileName 传入文件名， outfileName 输出文件名
    * 返回值，如果返回 0为处理正常，AV_EXIT_NORMAL（值为1则不需要处理） 其他则异常
    */
    int qtFastStart(const char *infileName, const char *outfileName);
private:

};