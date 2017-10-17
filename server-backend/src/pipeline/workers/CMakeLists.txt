add_subdirectory(ocrworker)
set(ocrworker_SRCS "${ocrworker_SRCS}" PARENT_SCOPE)

add_subdirectory(backgroundsegmentationworker)
set(bgsegmentworker_SRCS "${bgsegmentworker_SRCS}" PARENT_SCOPE)

add_subdirectory(textgroupcollateworker)
set(textgroupcollateworker_SRCS "${textgroupcollateworker_SRCS}" PARENT_SCOPE)

add_subdirectory(regexworker)
set(regexworker_SRCS "${regexworker_SRCS}" PARENT_SCOPE)

add_subdirectory(wordsplitworker)
set(wordsplitworker_SRCS "${wordsplitworker_SRCS}" PARENT_SCOPE)

add_subdirectory(spellcorrect)
set(spellcorrectworker_SRCS "${spellcorrectworker_SRCS}" PARENT_SCOPE)

add_subdirectory(votingtextmergeworker)
set(votingtextmergeworker_SRCS "${votingtextmergeworker_SRCS}" PARENT_SCOPE)

add_subdirectory(naivesemanticanalysis)
set(semanticanalysis_SRCS "${semanticanalysis_SRCS}" PARENT_SCOPE)

set(GLOBAL_INCLUDES 
    ${ocrworker_INCLUDES} 
    ${bgsegmentworker_INCLUDES} 
    ${logging_INCLUDES} 
    ${textgroupcollateworker_INCLUDES}
    ${regexworker_INCLUDES}
    ${wordsplitworker_INCLUDES}
    ${spellcorrectworker_INCLUDES}
    ${votingtextmergeworker_INCLUDES}
    ${semanticanalysis_INCLUDES}
    PARENT_SCOPE)
set(GLOBAL_LIBS 
    ${ocrworker_LIBS} 
    ${bgsegmentworker_LIBS} 
    PARENT_SCOPE)
