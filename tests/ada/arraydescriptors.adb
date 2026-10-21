with Ada.Text_IO;
with Array_Descriptor_Checks;
procedure ArrayDescriptors is
begin
    Array_Descriptor_Checks.Run;
    Ada.Text_IO.Put_Line ("wide array descriptors");
end ArrayDescriptors;
