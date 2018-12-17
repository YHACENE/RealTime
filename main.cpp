#include <stdio.h>
#include <string>
#include <opencv2/opencv.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <cmath>
#include <semaphore.h>

using namespace cv;

/* CONSTANTES *****************************************************************/

#define IMG_W 550
#define IMG_H 550
#define MAX_ITER 200
#define MAX_NORM 4        // 2

typedef struct Name
{
    long double LEFT = -1.0;
    long double RIGHT =  1.0;
    long double TOP = -1.0;
    long double BOTTOM = 1.0;
}Limit;

Limit limit;

long double zoom = 20.0;  // 20%

int longueurCouleur = 255;
int nombreTotalCouleurs = 7;
int nombreCouleur 	= 5;
int decalageCouleur = 0;

pthread_mutex_t mutex;
int cur_fragment = -1;
sem_t * sems = NULL;
int nthreads = 5, nfragments = 35;
int jr [IMG_H][IMG_W];
int threadmask [IMG_H][IMG_W];
int * thech;

int minj, maxj;

//long double pzoom = 20.0;  // 20%

/* MANIPULER LES NOMBRES COMPLEXES ********************************************/

enum BouttonActivation {
                         Actif,
                         Inactif
                       };

BouttonActivation BouttonCouleur = Inactif;
BouttonActivation BouttonComplex = Inactif;
BouttonActivation BouttonMask	 = Inactif;

typedef struct
{
    long double real;
    long double imag;
} complex;

complex c; // GLOBALE

typedef struct
{
	int idThread;
	int nFragments;
    complex c;
} ThreadData;

complex new_complex(long double real, long double imag)
{
    complex c;
    c.real = real;
    c.imag = imag;
    return c;
}

complex add_complex(complex a, complex b)
{
    a.real += b.real;
    a.imag += b.imag;
    return a;
}

complex mult_complex(complex a, complex b)
{
    complex m;
    m.real = a.real * b.real - a.imag * b.imag;
    m.imag = a.real * b.imag + a.imag * b.real;
    return m;
}

long double module_complex(complex c)
{
    return c.real * c.real + c.imag * c.imag;
}

/* FRACTALE DE JULIA *****************************************************/

complex convert(int x, int y)
{
   return new_complex(((long double) x / IMG_W * (limit.RIGHT - limit.LEFT)) + limit.LEFT, ((long double) y / IMG_H * (limit.BOTTOM - limit.TOP)) + limit.TOP);
}

int juliaDot(complex c, complex z, int iter)
{
	int i;
    for (i = 0; i < iter; i++)
    {
        z = add_complex(mult_complex(z, z), c);
        long double norm = module_complex(z);
        if (norm > MAX_NORM)
        {
            break;
        }
    }
    return i;
}

cv::Vec3b getColor(int H)
{
	int R = 0, G = 0, B = 0;
	int S = longueurCouleur;
	if(H > S * 7)
    {
		G = S - (H - (S * 7));
		B = S;
		R = S;
	}else
    {
		if(H > S * 6)
        {
			B = S - (H - (S * 6));
			R = S;
		}else
        {
			if(H > S * 5)
            {
				B = S;
				R = H - (S * 5);
			} else
            {
				if(H > S * 4)
                {
					G = S - (H - (S * 4));
					B = S;
				}else
                {
					if(H > S * 3)
                    {
						G = S ;
						B = H - (S * 3);
					}else
                    {
						if(H > S * 2)
                        {
							R = S - (H - (S * 2));
							G = S;
						}else{
							if(H > S * 1)
                            {
								R = S;
								G = H - S;
							}else
                            {
								R = H;
							}
						}
					}
				}
			}
		}
	}
	cv::Vec3b mycolor(B,G,R);
	return mycolor;
}

void draw_infos(cv::Mat& img)
{
	cv::line( img, Point(0,IMG_H+25), Point(IMG_W,IMG_H+25), Scalar(0,0,0), 50, 8);

	if(BouttonCouleur == Actif)
    {
		for(int i=0;i<IMG_W;i++)
		{
				int j = i * longueurCouleur * nombreTotalCouleurs / IMG_W;
				cv::Vec3b v = getColor(j);
				line( img, Point(i,IMG_H+1), Point(i,IMG_H+11), Scalar(v[0],v[1],v[2]), 1, 8);
		}
	}else
    {
		for(int i=0;i<IMG_W;i++)
		{
				int j = i * 255 / IMG_W;
				line( img, Point(i,IMG_H+1), Point(i,IMG_H+11), Scalar(j,j,j), 1, 8);
		}
	}

    int debut, fin, debut2, fin2;
    debut = decalageCouleur * IMG_W / nombreTotalCouleurs;
    fin = (decalageCouleur + nombreCouleur) * IMG_W / nombreTotalCouleurs;

	debut2	= (minj * (fin-debut) / MAX_ITER);
	fin2	= (maxj * (fin-debut) / MAX_ITER);


	if (BouttonCouleur == Actif)
    {
		line( img, Point(debut,IMG_H+13), Point(fin,IMG_H+13), Scalar(255,255,255), 1, 8);
		line( img, Point(debut2+debut,IMG_H+15), Point(fin2+debut,IMG_H+15), Scalar(255,255,255), 1, 8);
	}

	char rt[600]="";
	sprintf(rt,"%.5Lf, %.5Lf i",c.real,c.imag);
	putText(img, rt, Point2f(8, IMG_H + 38), FONT_HERSHEY_PLAIN, 1,  Scalar(255,255,255,255), 0);

	if(BouttonComplex == Actif)
    {
		line( img, Point(IMG_W/2,0), Point(IMG_W/2,IMG_H-1), Scalar(255, 255, 255), 1, 8);
		line( img, Point(0,IMG_H/2), Point(IMG_W,IMG_H/2), Scalar(255, 255, 255), 1, 8 );
	}
}

void draw_fractal(cv::Mat& img)
{
	int H, S = longueurCouleur;
	minj = MAX_ITER, maxj = 0;

    for (int x = 0; x < IMG_W; x++) {
        for (int y = 0; y < IMG_H; y++) {
            maxj = std::max(maxj,jr[y][x]);
            minj = std::min(minj,jr[y][x]);

            int nj;
			if(BouttonMask == Actif)
            {
				nj = threadmask[y][x];
				if(BouttonCouleur == Actif)
                {
					H = nj * S * nombreCouleur / 255; //MAX_ITER;
					img.at<cv::Vec3b>(cv::Point(x, y)) = getColor(H+(S*decalageCouleur));
				}
				else
                {
					H = nj;
					cv::Vec3b color(H,H,H);
					img.at<cv::Vec3b>(cv::Point(x, y)) = color;
				}
			}
			else
            {
				nj = jr[y][x];
				if(BouttonCouleur == Actif)
                {
					H = nj * S * nombreCouleur / MAX_ITER;
					img.at<cv::Vec3b>(cv::Point(x, y)) = getColor(H+(S*decalageCouleur));
				}
				else {
					H = nj * 255 / MAX_ITER;
					cv::Vec3b color(H,H,H);
					img.at<cv::Vec3b>(cv::Point(x, y)) = color;
				}
			}
            //int nj = (j-minj) * MAX_ITER / (maxj - minj);
        }
    }


	if(BouttonMask == Actif)
    {
		char ech[30]="";
		for (int e = 0; e < nfragments; e++)
        {
			sprintf(ech,"Ech %d => Th %d",e,thech[e]);
			int y = (int)(e*((float)IMG_H/nfragments))+14;
			putText(img, ech, Point2f(8, y), FONT_HERSHEY_PLAIN, 1,  Scalar(255,255,255,255), 0);
			//line( img, Point(0,y), Point(IMG_W,y), Scalar(255, 255, 255), 1, 8);
		}
	}
	draw_infos(img);
}

bool ask_task (int idThread, int & fragment, int nechs)
{
	pthread_mutex_lock(&mutex);
	if(cur_fragment < nechs) fragment = ++cur_fragment;
	bool task = (cur_fragment < nechs);
	if(task)
    {
		std::cout << "thread " << idThread << "=>" << fragment << std::endl;
		thech[fragment]	= idThread;
	}
	pthread_mutex_unlock(&mutex);
	return task;
}

void * ThreadJulia(void * data)
{
	int fragment;
	while(ask_task(((ThreadData*)data)->idThread,fragment,((ThreadData*)data)->nFragments))
    {
		long double ech = (long double) IMG_W * IMG_H / ((ThreadData*)data)->nFragments;
		int first = std::ceil(fragment * ech);
		int last  = std::ceil((fragment+1) * ech);

		for(int i=first; i<last; i++)
        {
			int x = i % IMG_W;
			int y = i / IMG_W;
			jr[y][x] = juliaDot(((ThreadData*)data)->c,convert(x, y), MAX_ITER);
			threadmask[y][x] = fragment * 255 / ((ThreadData*)data)->nFragments ;
		}
	}
	return NULL;
}


void julia(cv::Mat& img, int nths, int nechs, complex c )
{
	pthread_t * ths = (pthread_t*) malloc(nths * sizeof(pthread_t));
	pthread_mutex_init(&mutex,NULL);
	cur_fragment = -1;

	for(int i = 0; i<nths; i++)
	{
		ThreadData * data = (ThreadData*) malloc(sizeof(ThreadData));
		data->idThread = i;
		data->nFragments = nechs;
		data->c = new_complex(c.real,c.imag);

		pthread_create(&(ths[i]), NULL, ThreadJulia, data);
	}

	for(int i = 0; i < nths; i++)
	{
		pthread_join(ths[i], NULL);
	}
	free(ths);
	draw_fractal(img);
}


/* MAIN ***********************************************************************/

cv::Mat newImg(IMG_H+50, IMG_W, CV_8UC3);

void mouse_callback(int  event, int  x, int  y, int  flag, void *param)
{
    if (event == EVENT_MOUSEMOVE)
    {
		if(BouttonComplex == Actif)
        {
			if(y<=IMG_H)
            {
				c = new_complex((float)(x-(IMG_W/2))/(IMG_W/2), (float)(y-(IMG_H/2))/(IMG_H/2));
				julia(newImg,nthreads,nfragments,c);
				imshow("Window", newImg); // met Ã  jour l'image
			}
		}
    }
}


int main(int argc, char * argv[])
{
    namedWindow( "Window", WINDOW_AUTOSIZE );
    setMouseCallback("Window", mouse_callback);
    thech =	(int*) malloc(nfragments * sizeof(int));

	for(int p=0;p<argc;p++)
		std::cout << argv[p] << std::endl;

    int i = 0;
    c = new_complex(-0.175, 0.655);
    c = new_complex(0.0575, 0.6225);
    c = new_complex(-0.02104208416833675, 0.6462550100200402);
    c = new_complex(-0.7817635270541083, 0.13042334669338684);
    c = new_complex(0.2707414829659318, 0.0050475951903807825);
    c = new_complex(0.06, 0.624);

    c = new_complex(0.285, 0.01);
    c = new_complex(0.3, 0.5);
    c = new_complex(-1.41702285618, 0);
    c = new_complex(0.285, 0.013);
    c = new_complex(-0.052, -0.692);
    c = new_complex(-0.008, -0.64);
    c = new_complex(-0.74, -0.144);
    c = new_complex(0.26, 0.49);
    c = new_complex(0.27, 0.48667);
    c = new_complex(0.11, 0.6);
    c = new_complex(0.08333, 0.61333);
    julia(newImg,nthreads,nfragments,c);

    // interaction avec l'utilisateur
    int key = -1; // -1 indique qu'aucune touche est enfoncÃ©e
    char name[15];

    // on attend 30ms une saisie clavier, key prend la valeur de la touche
    // si aucune touche est enfoncÃ©e, au bout de 30ms on exÃ©cute quand mÃªme
    // la boucle avec key = -1, l'image est mise Ã  jour

    while( (key = waitKey(30)) )
    {
		//std::cout << "<" << (int)key << ">" << std::endl;
		long double LIMIT_LEFT_   = LIMIT_LEFT;
		long double LIMIT_RIGHT_  = LIMIT_RIGHT;
		long double LIMIT_TOP_    = LIMIT_TOP;
		long double LIMIT_BOTTOM_ = LIMIT_BOTTOM;

		switch ((int) key)
        {
			case (int) 's': // Sauvgarder l'image
				sprintf(name, "image_%d.bmp", i++);
				imwrite(name, newImg); // sauve une copie de l'image
				break;
		    case (int) 't': // mask des threads
				if (BouttonMask == Actif) BouttonMask = Inactif;
				else BouttonMask = Actif;
				draw_fractal(newImg);
				break;

			case (int) 'c': // changer le mode noir et blanc a couleur
				if (BouttonCouleur == Actif) BouttonCouleur = Inactif;
				else BouttonCouleur = Actif;
				draw_fractal(newImg);
				break;

			case (int) 'm': // changer le mode noir et blanc a couleur
				if (BouttonComplex == Actif) BouttonComplex = Inactif;
				else BouttonComplex = Actif;
				draw_fractal(newImg);
				break;

			case (int) 'z': // Zoom interieur
				LIMIT_LEFT		+= (LIMIT_RIGHT_ - LIMIT_LEFT_) / zoom;
				LIMIT_RIGHT		-= (LIMIT_RIGHT_ - LIMIT_LEFT_) / zoom;
				LIMIT_TOP		+= (LIMIT_BOTTOM_ - LIMIT_TOP_) / zoom;
				LIMIT_BOTTOM	-= (LIMIT_BOTTOM_ - LIMIT_TOP_) / zoom;

				julia(newImg,nthreads,nfragments,c);
				break;

			case (int) 'a': // Zoom exterieur
				LIMIT_LEFT -= (LIMIT_RIGHT_ - LIMIT_LEFT_) / zoom;
				LIMIT_RIGHT += (LIMIT_RIGHT_ - LIMIT_LEFT_) / zoom;
				LIMIT_TOP -= (LIMIT_BOTTOM_ - LIMIT_TOP_) / zoom;
				LIMIT_BOTTOM += (LIMIT_BOTTOM_ - LIMIT_TOP_) / zoom;

				julia(newImg,nthreads,nfragments,c);
				break;

			case 65362: // touche direction haut
				LIMIT_TOP -= (LIMIT_BOTTOM_ - LIMIT_TOP_) / zoom;
				LIMIT_BOTTOM -= (LIMIT_BOTTOM_ - LIMIT_TOP_) / zoom;
				julia(newImg,nthreads,nfragments,c);
				break;

			case 65364: // touche direction bas
				LIMIT_TOP += (LIMIT_BOTTOM_ - LIMIT_TOP_) / zoom;
				LIMIT_BOTTOM += (LIMIT_BOTTOM_ - LIMIT_TOP_) / zoom;
				julia(newImg,nthreads,nfragments,c);
				break;

			case 65361: // touche direction gauche
				LIMIT_LEFT -= (LIMIT_RIGHT_ - LIMIT_LEFT_) / zoom;
				LIMIT_RIGHT -= (LIMIT_RIGHT_ - LIMIT_LEFT_) / zoom;
				julia(newImg,nthreads,nfragments,c);
				break;

			case 65363: // touche direction droit
				LIMIT_LEFT += (LIMIT_RIGHT_ - LIMIT_LEFT_) / zoom;
				LIMIT_RIGHT += (LIMIT_RIGHT_ - LIMIT_LEFT_) / zoom;
				julia(newImg,nthreads,nfragments,c);
				break;

			case (int) 'o': // Decaler couleur a droite
				if (decalageCouleur + nombreCouleur < 7) decalageCouleur++;
				draw_fractal(newImg);
				break;

			case (int) 'i': // Decaler couleur a gauche
				if (decalageCouleur > 0) decalageCouleur--;
				draw_fractal(newImg);
				break;

			default:
				break;
			}

		if (key == 'q') break; // quitter la boucle while donc quitter le programme

		draw_infos(newImg);
		imshow("Window", newImg); // met Ã  jour l'image

    }

    destroyWindow("Window");
    return 0;
}
