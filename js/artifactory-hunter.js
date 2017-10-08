var ArtifactoryHunter = ArtifactoryHunter || {};

ArtifactoryHunter.config = {
    apiUrl: "https://oss.jfrog.org/artifactory/api",
}

ArtifactoryHunter.setConfig = function(config) {
    for (var key in config) {
        ArtifactoryHunter.config[key] = config[key];
    }
}

/* Returns: object with keys as names, values as dictionaries:
 *   {
 *     "type"        : "file" or "dir
 *     "url"         : url,
 *     "path"        : path,
 *   }
 */
ArtifactoryHunter.listDirectory = function(path, callback) {
    $.getJSON(ArtifactoryHunter.config.apiUrl + "/storage" + path, function( data ) {
        listings = {};
        if (data.hasOwnProperty("children") && data.hasOwnProperty("uri") && data.hasOwnProperty("path")) {
            for (var i = 0; i < data["children"].length; i++) {
                if (data["children"][i].hasOwnProperty("uri") &&
                    data["children"][i].hasOwnProperty("folder")
                    ) {
                    listings[data["children"][i]["uri"].substr(1)] = {
                        type:         (data["children"][i].hasOwnProperty("folder") && data["children"][i]["folder"]) ? "dir" : "file",
                        url:          data["uri"] + data["children"][i]["uri"],
                        path:         "/" + data["repo"] + data["path"] + data["children"][i]["uri"]
                    };
                } else {
                    console.warn("WARNING: API error may have occurred, API has been updated, or data is corrupt. Children data returned:");
                    console.warn(data["children"][i]);
                }
            }
        } else {
            if (data.hasOwnProperty("errors")) {
                console.error("ERROR: API Error returned:" + data["errors"]);
            } else {
                console.warn("WARNING: API error may have occurred, API has been updated, or data is corrupt.");
            }
        }
        callback(listings);
    }).fail(function() {
        console.error("ERROR: Could not load API endpoint.");
        callback({});
    });
}

/* Returns: object:
 *   {
 *     "path"        : path,
 *     "url"         : url,
 *     "mimeType"    : mimeType,
 *     "size"        : size,
 *     "md5"         : md5,
 *     "sha1"        : sha1,
 *     "createdDate" : created_date,
 *     "createdBy"   : created_by,
 *     "modifiedDate": modified_date,
 *     "modifiedBy"  : modified_by
 *   }
 */
ArtifactoryHunter.identifyFile = function(path, callback) {
    $.getJSON(ArtifactoryHunter.config.apiUrl + "/storage" + path, function( data ) {
        fileInfo = {};
        if (data.hasOwnProperty("path") &&
            data.hasOwnProperty("downloadUri") &&
            data.hasOwnProperty("mimeType") &&
            data.hasOwnProperty("size") &&
            data.hasOwnProperty("checksums") &&
            data.hasOwnProperty("originalChecksums") &&
            data.hasOwnProperty("created") &&
            data.hasOwnProperty("createdBy") &&
            data.hasOwnProperty("lastModified") &&
            data.hasOwnProperty("modifiedBy")
            ) {
            
            /* Quick sanity check */
            if (data["checksums"].hasOwnProperty("md5") &&
                data["originalChecksums"].hasOwnProperty("md5") &&
                (data["checksums"]["md5"] != data["originalChecksums"]["md5"])) {
                console.warn("WARNING: Checksum and original MD5 checksum do not match and will not be returned for path: " + path);
            }
            if (data["checksums"].hasOwnProperty("sha1") &&
                data["originalChecksums"].hasOwnProperty("sha1") &&
                (data["checksums"]["sha1"] != data["originalChecksums"]["sha1"])) {
                console.warn("WARNING: Checksum and original SHA1 checksum do not match and will not be returned for path: " + path);
            }
            
            fileInfo = {
                path:         data["path"],
                url:          data["downloadUri"],
                mimeType:     data["mimeType"],
                size:         data["size"],
                md5:          (data["checksums"].hasOwnProperty("md5") &&
                              data["originalChecksums"].hasOwnProperty("md5") &&
                              (data["checksums"]["md5"] == data["originalChecksums"]["md5"])) ? data["checksums"]["md5"] : null,
                sha1:         (data["checksums"].hasOwnProperty("sha1") &&
                              data["originalChecksums"].hasOwnProperty("sha1") &&
                              (data["checksums"]["sha1"] == data["originalChecksums"]["sha1"])) ? data["checksums"]["sha1"] : null,
                createdDate:  data["created"],
                createdBy:    data["createdBy"],
                modifiedDate: data["lastModified"],
                modifiedBy:   data["modifiedBy"]
            };
        } else {
            if (data.hasOwnProperty("errors")) {
                console.error("ERROR: API Error returned:" + data["errors"]);
            } else {
                console.warn("WARNING: API error may have occurred, API has been updated, or data is corrupt.");
            }
        }
        callback(fileInfo);
    }).fail(function() {
        console.error("ERROR: Could not load API endpoint.");
        callback({});
    });
}

/******/
ArtifactoryHunter.listDirectoryEnhanced = function(path, callback) {
    ArtifactoryHunter.listDirectory(path, function(res) {
        var count = 0;
        var fileNames = [];
        for (var name in res) {
            count++;
            if (res[name]["type"] == "file") {
                fileNames.push(name);
            }
        }
        
        if (!count) {
            callback({});
            return;
        }
        
        if (fileNames.length == 0) {
            callback(res);
            return;
        }
        
        ArtifactoryHunter.listDirectoryEnhancedRollyPolly(path, res, fileNames, -1, callback);
    });
};

ArtifactoryHunter.fileMetadataVars = [ "url", "mimeType", "size", "md5", "sha1", "createdDate", "createdBy", "modifiedDate", "modifiedBy" ];

ArtifactoryHunter.listDirectoryEnhancedRollyPolly = function(path, res, fileNames, currentFileIdx, callback, newRes = null) {
    if (newRes) {
        curFileName = fileNames[currentFileIdx];
        
        /* Update fields */
        for (var i = 0; i < ArtifactoryHunter.fileMetadataVars.length; i++) {
            res[curFileName][ArtifactoryHunter.fileMetadataVars[i]] = newRes[ArtifactoryHunter.fileMetadataVars[i]];
        }
    }
    
    currentFileIdx++;
    
    if (currentFileIdx >= fileNames.length) {
        callback(res);
        return;
    }
    
    ArtifactoryHunter.identifyFile(res[fileNames[currentFileIdx]]["path"], function(newNewRes) {
        ArtifactoryHunter.listDirectoryEnhancedRollyPolly(path, res, fileNames, currentFileIdx, callback, newNewRes);
    });
};
