#
# Prints the list List, one item per line. The Mode argument is passed as the first
# argument to each MESSAGE call. ${HeaderLine} is printed prior to the list.
#
function(message_list Mode List HeaderLine)
  message(${MODE} "${HeaderLine}")
  foreach (item ${List})
    message(${MODE} "    ${item}")
  endforeach()
  message(${MODE} "  (end of list)")
endfunction()
