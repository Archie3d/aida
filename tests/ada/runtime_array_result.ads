with Runtime_Array_Library;
pragma Elaborate_All (Runtime_Array_Library);
package Runtime_Array_Result is new Runtime_Array_Library.Capture
    (Runtime_Array_Library.Make, Runtime_Array_Library.B);
