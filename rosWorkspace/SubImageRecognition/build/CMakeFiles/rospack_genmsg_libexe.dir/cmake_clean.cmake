FILE(REMOVE_RECURSE
  "../msg_gen"
  "../srv_gen"
  "../src/SubImageRecognition/msg"
  "../src/SubImageRecognition/srv"
  "../msg_gen"
  "../srv_gen"
  "CMakeFiles/rospack_genmsg_libexe"
)

# Per-language clean rules from dependency scanning.
FOREACH(lang)
  INCLUDE(CMakeFiles/rospack_genmsg_libexe.dir/cmake_clean_${lang}.cmake OPTIONAL)
ENDFOREACH(lang)
