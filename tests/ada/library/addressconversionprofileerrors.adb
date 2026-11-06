with System.Address_To_Access_Conversions;
procedure AddressConversionProfileErrors is
    package P is new System.Address_To_Access_Conversions (Integer);
    Value : P.Object_Pointer := P.To_Pointer;
begin
    null;
end AddressConversionProfileErrors;
