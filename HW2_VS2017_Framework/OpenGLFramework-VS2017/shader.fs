#version 330 core
#define PI 3.14159265

out vec4 FragColor;
in vec3 vertex_color;
in vec3 FragPos;
in vec3 Normal;
in vec3 LightPos;
// ------------customize vars--------------
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform int ShadingType; //0=gouraud 1=phong
uniform int LightMode; // 0=Directional light  1=Position(point) light 2=Spot light
uniform vec3 viewPos;

struct Light  {
  float angle;
  vec3 position;     
  vec3 diffuse;     
};
uniform Light light;

struct Material  {
  vec3 Ka;            
  vec3 Kd;          
  vec3 Ks;           
  float shininess;  
};
uniform Material material;

// ---for attenuation---
vec3 result;
float constant;
float linear;
float quadratic;
float distance;
float attenuation;
float theta;
// ------------customize end--------------

void main() {
	// [TODO]
	if(ShadingType==0){
	FragColor = vec4(vertex_color, 1.0f);
	}
	else if (ShadingType==1){
    // gouraud shading
    // ------------------------
            if(LightMode==0){
	            // ambient
                vec3 ambient = vec3(0.15f, 0.15f, 0.15f) * material.Ka;
  	
                // diffuse 
	            vec3 norm = normalize(Normal);
                vec3 lightDir = normalize( - vec3(-1.0f,-1.0f, -1.0f)  );
                float diff = max(dot(norm, lightDir), 0.0);
                vec3 diffuse = light.diffuse *  material.Kd  *  diff ;

                // specular
                vec3 lightspecular= light.diffuse;
                vec3 viewDir = normalize(viewPos - FragPos);
                vec3 halfwayDir = normalize(lightDir + viewDir);
                float spec = pow(max(dot(norm, halfwayDir), 0.0), material.shininess);
                vec3 specular = lightspecular * (spec * material.Ks);  

                result =   ambient+ diffuse+specular ;
                FragColor = vec4(result, 1.0f);

            }
            if(LightMode==1){
	                // ambient
                vec3 ambient = vec3(0.15f, 0.15f, 0.15f) * material.Ka;
  	
                // diffuse 
	                vec3 norm = normalize(Normal);
                vec3 lightDir = normalize(LightPos-FragPos);
                float diff = max(dot(norm, lightDir), 0.0);
                vec3 diffuse = light.diffuse *  material.Kd  *  diff ;

                // specular
                vec3 lightspecular= light.diffuse;
                vec3 viewDir = normalize(viewPos - FragPos);
                vec3 halfwayDir = normalize(lightDir + viewDir);
                float spec = pow(max(dot(norm, halfwayDir), 0.0), material.shininess);
                vec3 specular = lightspecular * (spec * material.Ks);  

                //attenuation
                constant=0.01;
                linear=0.8;
                quadratic=0.1;
                distance = length(LightPos - FragPos);
                attenuation = min( 1.0f / (constant + linear*distance +quadratic*(distance*distance)),1);   

                result =   ambient*attenuation+ diffuse*attenuation+specular*attenuation ;
                FragColor = vec4(result, 1.0f);

            }
            if(LightMode==2){
	                // ambient
                vec3 ambient = vec3(0.15f, 0.15f, 0.15f) * material.Ka;
  	
                // diffuse 
	                vec3 norm = normalize(Normal);
                vec3 lightDir = normalize(LightPos-FragPos);
                float diff = max(dot(norm, lightDir), 0.0);
                vec3 diffuse = light.diffuse *  material.Kd  *  diff ;

                // specular
                vec3 lightspecular= light.diffuse;
                vec3 viewDir = normalize(viewPos - FragPos);
                vec3 halfwayDir = normalize(lightDir + viewDir);
                float spec = pow(max(dot(norm, halfwayDir), 0.0), material.shininess);
                vec3 specular = lightspecular * (spec * material.Ks);  

                //attenuation
                constant=0.05;
                linear=0.3;
                quadratic=0.6;
                distance = length(LightPos -FragPos );
                theta = dot(lightDir, normalize(-vec3(0,0,-1))); 
                float spoteffect = pow(max(theta, 0), 50);

                if(theta > cos( radians (light.angle ) )){
                     attenuation = min(1.0f / (constant + linear*distance +quadratic*(distance*distance)),1); 
                     result = spoteffect* ( ambient*attenuation+ diffuse*attenuation+specular*attenuation) ;

                }else{
                     result =   spoteffect*ambient ;
                }

                FragColor = vec4(result, 1.0f);
                
            }
	}
}

