#!/usr/bin/awk -f
{
   time = gensub(/^.* ([[:digit:]]+)$/, "\\1", 1)
   time = time / 1000000000
   proc = $7
   sub(/.*:/, "", proc)
   labl = gensub(/^.* IO-bench \S+ (.*) nanosec.*$/, "\\1", 1)

   marker = ">"
   color = "#888888"
   if (labl == "start") color = "#0000ff"
   else if (labl == "stop") marker = "<"
   else if (proc == "do_mpage_readpage") {
      if (labl == "alloc_new") color = "#00ff00"
      else if (labl == "confused") color = "#ff0000"
   } else if (proc == "do_generic_file_read") {
      if (labl == "page_loop") color = "#0000a0"
      else if (labl == "find_page") color = "#add8e6"
      else if (labl == "page_ok") color = "#00ff00"
      else if (labl == "page_not_up_to_date") color = "#800080"
      else if (labl == "page_not_up_to_date_locked") color = "#ff00ff"
      else if (labl == "readpage") color = "brown"
      else if (labl == "readpage_error") color = "#ff0000"
      else if (labl == "no_cached_page") color = "#800000"
   } else if (proc == "file_read_actor") {
      if (labl == "kmap_atomic") color = "#00ff00"
      else if (labl == "kmap") color = "#ff0000"
   } else if (proc == "generic_file_aio_read") {
      if (labl == "direct") color = "#ff0000"
      else if (labl = "regular") color = "#00ff00"
   }
   graph = ""
   if (marker == ">")
      graph = marker proc " " color
   else
      graph = marker proc

   printf "%10.6f %s\n", time, graph
}
