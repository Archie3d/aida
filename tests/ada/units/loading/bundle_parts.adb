with Bundle_Provider;
package Bundle_Consumer is
    Value : Integer := Bundle_Provider.Value + 1;
end Bundle_Consumer;

package Bundle_Provider is
    Value : Integer := 41;
end Bundle_Provider;
