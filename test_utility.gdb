
define reload_image
    monitor  reset
    monitor  halt
    load
end



define report_test_result
    set print address on
    set $currTestNode = testloggerlist->head
    set $null         = 0
    set $num_of_tests = 0
    set $num_of_fails = 0

    print "------- start of error report -------"
    while ($currTestNode != $null)
        if ($currTestNode->failFlag != 0)
            print  "[file path]: "
            print  $currTestNode->filepath
            print  "[line number]: "
            print  $currTestNode->lineNumber
            print  "[description]: "
            print  $currTestNode->description
            if (($currTestNode->compare_condition | 0x1) == 0x1)
                print  "[compare type]: equal-to"
            end
            if (($currTestNode->compare_condition | 0x2) == 0x1)
                print  "[compare type]: greater-than"
            end
            if (($currTestNode->compare_condition | 0x4) == 0x1)
                print  "[compare type]: less-than"
            end
            if ($currTestNode->compare_condition == 0x0)
                print  "[compare type]: in-range"
            end
            if ($currTestNode->compare_condition == 0x0)
                print  "[range]: (represented as pointer) "
                print  /x  $currTestNode->expectedValue[0]
                print  /x  $currTestNode->expectedValue[1]
            else
                print  "[expected value]: (represented as pointer) "
                print  /x  $currTestNode->expectedValue[0]
            end
            print  "[actual value]: (represented as pointer) "
            print  /x  $currTestNode->actualValue
            print ""
            set $num_of_fails += 1
        end
        set $num_of_tests += 1
        set $currTestNode  = $currTestNode->next
    end
    print "------- end of error report -------"
    print ""
    print "[number of tests]:"
    print $num_of_tests 
    print "[number of failure]:"
    print $num_of_fails
end




define report_mpu_region
    set *0xe000ed98 = $arg0
    x/3xw 0xe000ed98
end




file     build/FreeRTOS-ESP8266-AT-lib.elf
target   remote localhost:3333
reload_image
break    TestEnd
info     b


