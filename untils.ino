String getValue(String data, char separator, int index)
{
  int found = 0;
  int strIndex[] = {
    0, -1        };
    
   //data = data+"|dummy";
  int maxIndex = data.length()-1;
  
 

  for(int i=0; i<=maxIndex && found<=index; i++){
    if(data.charAt(i)==separator || i==maxIndex){
      found++;
      strIndex[0] = strIndex[1]+1;
      strIndex[1] = (i == maxIndex) ? i+1 : i;
    }
  }
//  String res = data.substring(strIndex[0], strIndex[1]);
 // if (res.substring(-1, 1) == "|") res = res.substring(0, -1);
 
  return found>index ? data.substring(strIndex[0], strIndex[1]): "";
}
