with Ada.Text_IO; use Ada.Text_IO;
with Runtime_Array_Library; use Runtime_Array_Library;
with Runtime_Array_Saved;
with Runtime_Array_Result;
pragma Elaborate_All (Runtime_Array_Library);
procedure RuntimeArrayLibrary is
    Later : Text := Make;
    Later_Matrix : Shape := (others => (others => 9));
    Copy : Hidden := Make_Hidden;
begin
    if N /= 100 or Text'First /= 2 or Text'Last /= 3 or Text'Length /= 2
        or Derived'Last /= 3 or D (-1) /= 8 or Vector'Length /= 5 or Shape'Length (2) /= 2 or Indexed'Last /= 3
        or Empty'First /= 5 or Empty'Last /= 3 or Empty'Length /= 0
        or Inferred /= "aabc" or Later /= A or I (3) /= 6 or V (-1) /= 7 then
        raise Program_Error;
    end if;
    if Three.Vector'Length /= 3 or Four.Text'Last /= 4
        or Three.A (3) /= 3 or Four.A (4) /= 4 or Four.B /= "bbbb" then
        raise Program_Error;
    end if;
    Inspect ((11 .. 12 => 'a'), (7 .. 8 => (9 .. 10 => 9)));
    Check_Hidden (Copy);
    Runtime_Array_Saved.Check;
    if Runtime_Array_Result.Snapshot /= "aa" then raise Program_Error; end if;
    if B /= "zc" or View /= A then raise Program_Error; end if;
    View (2) := 'q';
    if A /= "qa" or Runtime_Array_Saved.Snapshot /= "aa" then raise Program_Error; end if;
    begin
        A := "bad";
        raise Program_Error;
    exception
        when Constraint_Error => null;
    end;
    Put_Line ("library runtime arrays ok");
end RuntimeArrayLibrary;
