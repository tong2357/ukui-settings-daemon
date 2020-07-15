#include <QCoreApplication>
#include <QApplication>

#include "xrandr-manager.h"
#include <syslog.h>

XrandrManager *XrandrManager::mXrandrManager = nullptr;

XrandrManager::XrandrManager()
{
    time = new QTimer(this);
}

XrandrManager::~XrandrManager()
{
    if(mXrandrManager){
        delete mXrandrManager;
        mXrandrManager = nullptr;
    }
    if(time){
        delete time;
        time = nullptr;
    }
}

XrandrManager* XrandrManager::XrandrManagerNew()
{
    if (nullptr == mXrandrManager)
        mXrandrManager = new XrandrManager();
    return mXrandrManager;
}

bool XrandrManager::XrandrManagerStart()
{
    qDebug("Xrandr Manager Start");
    connect(time,SIGNAL(timeout()),this,SLOT(StartXrandrIdleCb()));

    time->start();

    return true;
}

void XrandrManager::XrandrManagerStop()
{
    syslog(LOG_ERR,"Xrandr Manager Stop");
}

/**
 * @name ReadMonitorsXml();
 * @brief 读取monitors.xml文件
 */
bool XrandrManager::ReadMonitorsXml()
{
    int mNum = 0;
    QString homePath = getenv("HOME");
    QString monitorFile = homePath+"/.config/monitors.xml";
    QFile file(monitorFile);
    if(!file.open(QIODevice::ReadOnly))
            return false;

    QDomDocument doc;
    if(!doc.setContent(&file))
    {
        file.close();
        return false;
    }
    file.close();

    XmlFileTag.clear();
    mIntDate.clear();

    QDomElement root=doc.documentElement(); //返回根节点
    //qDebug()<<root.nodeName();
    QDomNode n=root.firstChild();

    while(!n.isNull())
    {
        if(n.isElement()){
            QDomElement e=n.toElement();
            QDomNodeList list=e.childNodes();

            for(int i=0;i<list.count();i++){
                QDomNode node=list.at(i);

                if(node.isElement()){
                    QDomNodeList e2 = node.childNodes();

                    if(node.toElement().tagName() == "clone"){
                        XmlFileTag.insert("clone",node.toElement().text());
                        qDebug()<<"clone:"<<node.toElement().tagName()<<node.toElement().text();
                    }else if("output" == node.toElement().tagName()){
                        XmlFileTag.insert("name",node.toElement().attribute("name"));
                        for(int j=0;j<e2.count();j++){
                            QDomNode node2 = e2.at(j);

                            if(node2.toElement().tagName() == "width")
                                XmlFileTag.insert("width",node2.toElement().text());
                            else if(node2.toElement().tagName() == "height")
                                XmlFileTag.insert("height",node2.toElement().text());
                            else if("x" == node2.toElement().tagName())
                                XmlFileTag.insert("x",node2.toElement().text());
                            else if("y" == node2.toElement().tagName())
                                XmlFileTag.insert("y",node2.toElement().text());
                            else if(node2.toElement().tagName() == "primary")
                                XmlFileTag.insert("primary",node2.toElement().text());

                        }
                        mNum++;
                    }
                }
            }
        }
        n = n.nextSibling();
    }
    qDebug()<<"mNum = "<<mNum;
    mIntDate.insert("XmlNum",mNum);
    return true;
}

/**
 * @name SetScreenSize();
 * @brief 设置单个屏幕分辨率
 */
bool XrandrManager::SetScreenSize(Display  *dpy, Window root, int width, int height)
{
    SizeID          current_size;
    Rotation        current_rotation;
    XRRScreenSize   *sizes;
    XRRScreenConfiguration *sc;
    int     nsize;
    int     size = -1;
    int     rot  = -1;
    short   current_rate;
    double  rate = -1;
    int     reflection = 0;

    sc = XRRGetScreenInfo (dpy, root);
    if (sc == NULL){
        syslog(LOG_ERR,"Screen configuration is Null");
        return false;
    }
    /* 配置当前配置 */
    current_size = XRRConfigCurrentConfiguration (sc, &current_rotation);
    sizes = XRRConfigSizes(sc, &nsize);

    for (size = 0;size < nsize; size++)
    {
        if (sizes[size].width == width && sizes[size].height == height)
            break;
    }

    if (size >= nsize) {
        syslog(LOG_ERR,"Size %dx%d not found in available modes\n", width, height);
        return false;
    }
    else if (size < 0)
        size = current_size;
    else if (size >= nsize) {
        syslog(LOG_ERR,"Size index %d is too large, there are only %d sizes\n", size, nsize);
        return false;
    }

    if (rot < 0) {
        for (rot = 0; rot < 4; rot++)
            if (1 << rot == (current_rotation & 0xf))
                break;
    }
    current_rate = XRRConfigCurrentRate (sc);
    if (rate < 0) {
        if (size == current_size)
            rate = current_rate;
        else
            rate = 0;
    }
    XSelectInput (dpy, root, StructureNotifyMask);
    Rotation rotation = 1 << rot;
    XRRSetScreenConfigAndRate (dpy, sc, root, (SizeID) size,
                               (Rotation) (rotation | reflection),
                               rate, CurrentTime);
    XRRFreeScreenConfigInfo(sc);
    return true;
}

/**
 * @name AutoConfigureOutputs();
 * @brief 自动配置输出,在进行硬件HDMI屏幕插拔时
 */
void XrandrManager::AutoConfigureOutputs (XrandrManager *manager, guint32 timestamp)
{
    MateRRConfig *config;
    MateRROutputInfo **outputs;
    int i;
    GList *just_turned_on;
    GList *l;
    int x;

    gboolean applicable;

    config = mate_rr_config_new_current (manager->rw_screen, NULL);
    just_turned_on = NULL;
    outputs = mate_rr_config_get_outputs (config);

    for (i = 0; outputs[i] != NULL; i++) {
        MateRROutputInfo *output = outputs[i];

        if (mate_rr_output_info_is_connected (output) && !mate_rr_output_info_is_active (output)) {
                mate_rr_output_info_set_active (output, TRUE);
                mate_rr_output_info_set_rotation (output, MATE_RR_ROTATION_0);
                just_turned_on = g_list_prepend (just_turned_on, GINT_TO_POINTER (i));
        } else if (!mate_rr_output_info_is_connected (output) && mate_rr_output_info_is_active (output))
                mate_rr_output_info_set_active (output, FALSE);
    }

    /* Now, lay out the outputs from left to right.  Put first the outputs
     * which remained on; put last the outputs that were newly turned on.
     * 现在，从左到右布置输出。 首先整理目前存在的输出, 最后放置新打开的输出。
     */
    x = 0;
    int XmlNum = manager->mIntDate.value("XmlNum");

    /* First, outputs that remained on
     * 首先，输出保持不变
     */
    for (i = 0; outputs[i] != NULL; i++) {
        MateRROutputInfo *output = outputs[i];

        if (g_list_find (just_turned_on, GINT_TO_POINTER (i)))
                continue;

        if (mate_rr_output_info_is_active (output)) {
            int width=0, height=0;
            char *name;

            g_assert (mate_rr_output_info_is_connected (output));
            name = mate_rr_output_info_get_name(output);

            for(int i=0;i<XmlNum;i++)
            {
                QString DisplayName = manager->XmlFileTag.values("name")[i];
                if(DisplayName == QString(name))
                {
                    width = manager->XmlFileTag.values("width")[i].toLatin1().toInt();
                    height = manager->XmlFileTag.values("height")[i].toLatin1().toInt();
                    //x = manager->XmlFileTag.values("height")[x].toLatin1().toInt();
                }
            }
            if(width ==0 ||height ==0)
                mate_rr_output_info_get_geometry (output, NULL, NULL, &width, &height);
            mate_rr_output_info_set_geometry (output, x, 0, width, height);

            //qDebug("First: x = %d, width=%d, height=%d name = %s", x, width, height,name);

            x += width;
        }
    }

    /* Second, outputs that were newly-turned on
     * 第二，新打开的输出
     */
    for (l = just_turned_on; l; l = l->next) {
        MateRROutputInfo *output;
        int width=0,height=0;
        char *name;
        i = GPOINTER_TO_INT (l->data);
        output = outputs[i];
        g_assert (mate_rr_output_info_is_active (output) && mate_rr_output_info_is_connected (output));

        /* since the output was off, use its preferred width/height (it doesn't have a real width/height yet) */
        width = mate_rr_output_info_get_preferred_width (output);
        height = mate_rr_output_info_get_preferred_height (output);
        name = mate_rr_output_info_get_name(output);

        for(int i=0;i<XmlNum;i++)
        {
            QString DisplayName = manager->XmlFileTag.values("name")[i];
            if(DisplayName == QString(name))
            {
                width = manager->XmlFileTag.values("width")[i].toLatin1().toInt();
                height = manager->XmlFileTag.values("height")[i].toLatin1().toInt();
                //x = manager->XmlFileTag.values("height")[x].toLatin1().toInt();
            }
        }
        if(width ==0 ||height ==0){
            width = mate_rr_output_info_get_preferred_width (output);
            height = mate_rr_output_info_get_preferred_height (output);
        }
        mate_rr_output_info_set_geometry (output, x, 0, width, height);

        qDebug("Second: x = %d, width=%d, height=%d name = %s", x, width, height,name);
        x += width;
    }

    /* Check if we have a large enough framebuffer size.  If not, turn off
     * outputs from right to left until we reach a usable size.
     * 检查我们是否有足够大的帧缓冲区大小。如果没有，从右到左关闭输出，直到达到可用大小为止。
     */
    just_turned_on = g_list_reverse (just_turned_on); /* now the outputs here are from right to left */
    l = just_turned_on;
    while (1) {
        MateRROutputInfo *output;
        applicable = mate_rr_config_applicable (config, manager->rw_screen, NULL);
        if (applicable)
                break;
        if (l) {
            i = GPOINTER_TO_INT (l->data);
            l = l->next;

            output = outputs[i];
            mate_rr_output_info_set_active (output, FALSE);
        } else
            break;
    }
    /* Apply the configuration!
     * 应用配置
     */
    if (applicable)
        mate_rr_config_apply_with_time (config, manager->rw_screen, timestamp, NULL);

    g_list_free (just_turned_on);
    g_object_unref (config);
}

/**
 * @brief XrandrManager::OnRandrEvent : 屏幕事件回调函数
 * @param screen
 * @param data
 */
void XrandrManager::OnRandrEvent(MateRRScreen *screen, gpointer data)
{
    unsigned int change_timestamp, config_timestamp;
    XrandrManager *manager = (XrandrManager*) data;

    /* 获取 更改时间 和 配置时间 */
    mate_rr_screen_get_timestamps (screen, &change_timestamp, &config_timestamp);

    //qDebug("change_timestamp=%d, config_timestamp=%d",change_timestamp,config_timestamp);

    if (change_timestamp >= config_timestamp) {
        /* The event is due to an explicit configuration change.
         * If the change was performed by us, then we need to do nothing.
         * If the change was done by some other X client, we don't need
         * to do anything, either; the screen is already configured.
         */
        qDebug()<<"Ignoring event since change >= config";
    } else {
        qDebug()<<__func__;
        manager->ReadMonitorsXml();
        manager->AutoConfigureOutputs (manager, config_timestamp);
    }
}
/**
 * @brief XrandrManager::StartXrandrIdleCb
 * 开始时间回调函数
 */
void XrandrManager::StartXrandrIdleCb()
{
    Display     *dpy;
    Window      root;
    QString ScreenName;
    int ScreenNum = 1;
    int width,  height;

    time->stop();

    ReadMonitorsXml();

    rw_screen = mate_rr_screen_new (gdk_screen_get_default (),NULL);
    if(rw_screen == nullptr){
        qDebug()<<"Could not initialize the RANDR plugin";
        return;
    }
    g_signal_connect (rw_screen, "changed", G_CALLBACK (OnRandrEvent), this);

    ScreenNum = QApplication::screens().length();
    dpy = XOpenDisplay(0);

    int screen = DefaultScreen(dpy);
    root = RootWindow(dpy, screen);

    int XmlNum = mIntDate.value("XmlNum");

    if(ScreenNum == 1){
        ScreenName = QApplication::screens().at(0)->name();
        for(int i=0;i<XmlNum;i++){
            QString DisplayName = XmlFileTag.values("name")[i];
            if(ScreenName == DisplayName){
                width = XmlFileTag.values("width")[i].toLatin1().toInt();
                height = XmlFileTag.values("height")[i].toLatin1().toInt();
            }
        }
        SetScreenSize(dpy, root, width, height);
    }
    /*else if(ScreenNum > 1){
        if(XmlFileTag.value("clone")=="true"){
            //
        }else{
            for(int i=0;i<XmlNum;i++){
                if(XmlFileTag.values("primary")[i] == "yes"){

                }


            }
        }
    }
    */
}



