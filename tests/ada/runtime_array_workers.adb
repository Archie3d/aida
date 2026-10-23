with Ada.Text_IO;
with Runtime_Array_Library; use Runtime_Array_Library;
package body Runtime_Array_Workers is
    protected body Counter is
        procedure Increment is
        begin
            Count := Count + 1;
        end Increment;
    end Counter;
    task body Worker is
        Copy : Text := Make;
    begin
        if Copy /= A or Cells'Length /= 4 or Workers'Length /= 4 then
            raise Program_Error;
        end if;
        for I in Cells'Range loop
            Cells (I).Increment;
        end loop;
        Ada.Text_IO.Put_Line ("library array worker passed");
    end Worker;
end Runtime_Array_Workers;
