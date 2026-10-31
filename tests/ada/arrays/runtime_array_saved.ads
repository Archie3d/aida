with Runtime_Array_Library;
pragma Elaborate_All (Runtime_Array_Library);
package Runtime_Array_Saved is new Runtime_Array_Library.Capture
    (Runtime_Array_Library.A, Runtime_Array_Library.B);
