#include "GRID_Joiner.h"

inline bool is_exists (const std::string& name) {
    struct stat buffer;
    return (stat (name.c_str(), &buffer) == 0);
}

#ifdef WIN32
std::vector<std::string> listFiles(const char* path, std::regex regexp){
    struct _finddata_t dirFile;
    intptr_t hFile;

    std::vector<std::string> files{};

    if (( hFile = _findfirst( path, &dirFile )) != -1 )
    {
        do
        {
            if ( !strcmp( dirFile.name, "."   )) continue;
            if ( !strcmp( dirFile.name, ".."  )) continue;

            //Ignore hidden
            if ( dirFile.attrib & _A_HIDDEN ) continue;
            if ( dirFile.name[0] == '.' ) continue;


            if ( std::regex_match(dirFile.name, regexp))
                files.emplace_back(dirFile.name);

        } while ( _findnext( hFile, &dirFile ) == 0 );
        _findclose( hFile );
    }
    return files;
}
#else
std::vector<std::string> listFiles( const char* path, std::regex regexp )
{
    DIR* dirFile = opendir( path );
    std::vector<std::string> files{};
    if ( dirFile )
    {
        struct dirent* hFile;
        errno = 0;
        while (( hFile = readdir( dirFile )) != NULL )
        {
         if ( !strcmp( hFile->d_name, "."  )) continue;
         if ( !strcmp( hFile->d_name, ".." )) continue;

         // in linux hidden files all start with '.'
         if ( hFile->d_name[0] == '.' ) continue;

         if ( std::regex_match(dirFile.name, regexp))
              files.emplace_back(dirFile.name);
        }
        closedir( dirFile );
    }
    return files;
}
#endif

std::string GRID_Utils::pathAppend(const std::string& p1, const std::string& p2) {
    char sep = '/';
#ifdef _WIN32
    sep = '\\';
#endif

    if (p1[p1.length()] != sep) { // Need to add a
        // path separator
        return(p1 + sep + p2);
    }
    else
        return(p1 + p2);
}

void GRID_Utils::mkdir_all(std::wstring path) {
    for(std::wstring::iterator it = path.begin(); it != path.end(); ++it){
        if(*it == '.') return; //found a file => abort
        char sep = '/';
#ifdef _WIN32
        sep = '\\';
#endif

        if(*it == '\\' || *it == '/'){
            hr_mkdir(std::wstring(path.begin(), it).c_str());
        }
    }
}

void GRID_Joiner::start() {
    active = true;
    parseProcessedImgs();

    while(active){
        std::cout << std::to_string(active) << std::endl << std::flush;
        std::vector<std::string> files = std::move(listFiles(work_folder.c_str(), std::regex(".image4f")));
        for(auto it = files.begin(); it != files.end(); ++it){
            if(processed_imgs.find(*it) == processed_imgs.end()){
                mergeImage(*it);
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(UPDATE_INTERVAL_MS));
    }

    std::cout << processed_imgs_xml << std::endl << std::flush;
    serializeProcessedImgs();
}

void GRID_Joiner::parseProcessedImgs() {
    if(!is_exists(result_img)) return;
    if(!is_exists(processed_imgs_xml)) return;

    pugi::xml_document doc;
    pugi::xml_parse_result res = doc.load_file(processed_imgs_xml.c_str());
    if(res.status != pugi::status_ok) return;

    for(pugi::xml_node merged = doc.child(L"merged"); merged; merged = merged.next_sibling(L"merged")){
        std::string res_file = pugi::as_utf8(merged.attribute(L"file").value());
        if(res_file != result_img) continue;

        for(pugi::xml_node img = merged.child(L"img"); img; img = img.next_sibling(L"img")){
            std::string img_file = pugi::as_utf8(img.attribute(L"file").value());
            processed_imgs.insert(img_file);
            processed_count = processed_imgs.size();
        }
    }
}

void GRID_Joiner::serializeProcessedImgs() {
    pugi::xml_document doc;
    pugi::xml_parse_result res = doc.load_file(processed_imgs_xml.c_str());

    pugi::xml_node existed = doc.find_child_by_attribute(L"file", pugi::as_wide(result_img).c_str());

    pugi::xml_node merged; merged.set_name(L"merged");
    for(std::set<std::string>::iterator it = processed_imgs.begin(); it != processed_imgs.end(); ++it){
        merged.append_child(L"img").append_attribute(L"file").set_value(pugi::as_wide(*it).c_str());
    }

    doc.remove_child(existed);
    doc.append_move(merged);
    doc.save_file(processed_imgs_xml.c_str());
}

size_t GRID_Joiner::getProcessed() const {
    return processed_count;
}

void GRID_Joiner::mergeImage(const std::string &str) {
    std::vector<float> res_img; int rw, rh;
    if(!is_exists(result_img)) res_img.resize(w*h, 0.0f);
    else{
        HydraRender::LoadImageFromFile(result_img, res_img, rw, rh);
        if(rw != w || h != rh){ //Wrong file size
            res_img.clear();
            res_img.resize(w*h, 0.0f);
            //Clear it all
            processed_imgs.clear();
            processed_count = processed_imgs.size();
        }
    }

    std::vector<float> added_img; int aw, ah;
    HydraRender::LoadImageFromFile(str, added_img, aw, ah);
    if(aw == w && h == ah){
        size_t coef = processed_imgs.size();
        std::transform(res_img.begin(), res_img.end(), added_img.begin(), res_img.begin(),
                [coef](float a, float b){return (a*coef+b)/(coef+1);});
        //res_img = (res_img * coef + added_img) / (coef + 1)

        processed_imgs.insert(str);
        processed_count = processed_imgs.size();
    }

    HydraRender::SaveHDRImageToFileHDR(result_img, w, h, res_img.data());
    std::string copy_img = result_img;
    copy_img.replace(copy_img.rfind('.',copy_img.length())+1, 4, "tiff");
    HydraRender::SaveHDRImageToFileHDR(copy_img, w, h, res_img.data());
    serializeProcessedImgs();
}

GRID_Joiner::GRID_Joiner(int w, int h, std::string work_folder, std::string merged_xml, std::string result_img) :
    w(w), h(h),
    work_folder(work_folder),
    active(false),
    processed_imgs(),
    processed_count(processed_imgs.size()),
    processed_imgs_xml(merged_xml),
    result_img(result_img)
    {
        using namespace GRID_Utils;
        mkdir_all(s2ws(work_folder));
        mkdir_all(s2ws(processed_imgs_xml));
        mkdir_all(s2ws(result_img));
}
