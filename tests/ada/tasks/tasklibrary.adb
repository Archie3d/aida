with Ada.Text_IO;
package body TaskLibrary is
    task body Worker is
    begin
        Ada.Text_IO.Put_Line ("library task passed");
    end Worker;
    procedure Check is
    begin
        null;
    end Check;
end TaskLibrary;
