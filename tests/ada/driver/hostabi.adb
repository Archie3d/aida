with Ada.Text_IO;
with System;
procedure HostAbi is
    type Unsigned is mod 2 ** 64;
    function Probe (Narrow : Integer; Wide : Long_Integer; Single : Float;
                    Real : Long_Float; Address : System.Address; Tail1 : Integer;
                    Tail2 : Long_Integer; Tail3 : Long_Float; Tail4 : Long_Integer;
                    Stack1 : Integer; Stack2 : Long_Integer; Stack3 : Integer;
                    Stack4 : Long_Integer) return Integer;
    pragma Import (C, Probe, "__ada_host_abi_check");
    function Mix (Value : Unsigned) return Unsigned;
    pragma Import (C, Mix, "__ada_host_abi_unsigned");
    function Echo (Value : System.Address) return System.Address;
    pragma Import (C, Echo, "__ada_host_abi_address");
    Value : aliased Long_Integer := -16#123456789#;
begin
    if Integer'Size /= 32 or else Long_Integer'Size /= 64 or else Float'Size /= 32
        or else Long_Float'Size /= 64 or else Character'Size /= 8
        or else Wide_Character'Size /= 16 or else Wide_Wide_Character'Size /= 32
        or else System.Address'Size /= 64 or else Duration'Size /= 64
        or else System.Storage_Unit /= 8 or else System.Word_Size /= 64 then
        raise Program_Error;
    end if;
    if Probe (Integer'First, 16#123456789ABCDEF#, 1.25, -2.5, Value'Address,
              37, -16#123456789ABCDEF#, 9.75, Long_Integer'Last,
              -77, 16#23456789ABCDEF#, Integer'Last, Long_Integer'First) /= 1
        or else Value /= 16#102030405060708#
        or else Mix (16#FEDCBA9876543210#) /= 16#7EDCBA9876543211#
        or else Echo (Value'Address) /= Value'Address then
        raise Program_Error;
    end if;
    Ada.Text_IO.Put_Line ("host ABI passed");
end HostAbi;
