#include <cmath>
#include <typeinfo>
#include <RcppArmadillo.h>
#include <progress.hpp>
#ifdef _OPENMP
#include <omp.h>
#endif
#include "update_param.h"
// [[Rcpp::depends(RcppProgress)]]
// [[Rcpp::depends(RcppArmadillo)]]
//' Run Markov-Chain Monte-Carlo sampling algorithm
//'
//' This function will generate samples from the posterior distribution.
//'
//' @param y A list of of length n containing responses
//' @param missing A list of length n containing missing indices for each
//' response
//' @param X An n by p design matrix
//' @param splineS Basis matrix for longitudinal direction
//' @param splineT Basis matrix for functional direction
//' @param q1 Number of latent factors for longitudinal direction
//' @param q2 Number of latent factors for functional direction
//' @param thin Keep every thin samples
//' @param iter Number of posterior samples to keep
//' @param burnin Number of burnin samples to discard for posterior inference
//' @param nchains How many chains to run
//' @export run_mcmc
//' @return A list of samples for each parameter. This list can be used as input
//' to eigenLFChains for further post-processing
//' @examples
//' See the example in Root/Example
// [[Rcpp::export]]
Rcpp::List run_mcmc(arma::field<arma::vec> y,
                          arma::field<arma::vec> missing, arma::mat X,
                          arma::mat splineS, arma::mat splineT, int q1,
                          int q2, int iter, int thin, int burnin, int nchains){
  // Allocate memory for parameters
  Progress p(iter, true);

  int p1 = splineT.n_cols;
  int p2 = splineS.n_cols;
  arma::field<arma::cube> LambdaF(nchains);
  arma::field<arma::cube> GammaF(nchains);
  arma::field<arma::cube> Phi1F(nchains);
  arma::field<arma::cube> Phi2F(nchains);
  arma::field<arma::mat> DM1F(nchains);
  arma::field<arma::mat> DM2F(nchains);
  arma::field<arma::cube> SigmaF(nchains);
  arma::field<arma::vec> varphiF(nchains);
  arma::field<arma::vec> a1LF(nchains);
  arma::field<arma::vec> a2LF(nchains);
  arma::field<arma::vec> a1GF(nchains);
  arma::field<arma::vec> a2GF(nchains);
  arma::field<arma::cube> BetaF(nchains);
  arma::field<arma::cube> HF(nchains);
  arma::field<arma::cube> EF(nchains);
  for(arma::uword i = 0; i < nchains; i++){
    LambdaF(i) = arma::cube(p1, q2, iter);
    GammaF(i) = arma::cube(p2, q1, iter);
    Phi1F(i) = arma::cube(p1, q2, iter);
    Phi2F(i) = arma::cube(p2, q1, iter);
    DM1F(i) = arma::mat(q2, iter);
    DM2F(i) = arma::mat(q1, iter);
    SigmaF(i) = arma::cube(p1, p2, iter);
    varphiF(i) = arma::vec(iter);
    a1LF(i) = arma::vec(iter);
    a2LF(i) = arma::vec(iter);
    a1GF(i) = arma::vec(iter);
    a2GF(i) = arma::vec(iter);
    BetaF(i) = arma::cube(X.n_cols, q1 * q2, iter);
    HF(i) = arma::cube(q1 * q2, q1 * q2, iter);
    EF(i) = arma::cube(q1 * q2, X.n_cols, iter);

  }
  arma::mat Lambda(p1, q2);
  arma::mat Gamma(p2, q1);
  arma::mat Phi1(p1, q2);
  arma::mat Phi2(p2,q1);
  arma::vec DM1(q2);
  arma::vec DM2(q1);
  arma::mat Sigma(p1,p2);
  double a1Ld = 1;
  double a2Ld = 1;
  double a1Gd = 1;
  double a2Gd = 1;
  double V;
  arma::mat Beta = arma::zeros(X.n_cols, q1 * q2);
  arma::mat H(q1 * q2, q1 * q2);
  arma::mat E(q1 * q2, X.n_cols);
  arma::field<arma::cube> etaF(nchains, iter);
  arma::field<arma::cube> thetaF(nchains, iter);
  for(int i = 0; i < nchains; i++){
    for(int j = 0; j < iter; j++){
      etaF(i,j) = arma::cube(q2, q1, X.n_rows);
      thetaF(i,j) = arma::cube(p1, p2, X.n_rows);
    }
    etaF(i, 0).randn();
    thetaF(i, 0).randn();
  }
  arma::cube Eta(q2, q1, X.n_rows);
  arma::cube Theta(p1, p2, X.n_rows);
  arma::mat imputedY(splineS.n_rows * splineT.n_rows, X.n_rows);
  for(arma::uword i = 0; i < X.n_rows; i++){
    imputedY.col(i) = initializeY(y(i), missing(i),
                 splineS.n_rows, splineT.n_rows);
  }
  arma::mat initialY = imputedY;
  // Set initial values
  Lambda.randn();
  Gamma.randn();
  H.eye();
  V = R::rgamma(1,1);
  Eta.randn();
  Theta.randn();
  Beta.randn();
  E.randu();
  Phi1.ones();//
  Phi2.ones();//
  Sigma.fill(.1);
  Beta.randn();
  E.ones();
  DM1.ones();
  DM2.ones();

  for(int k = 0; k < nchains; k++){
    for(int i = 0; i < iter; i++){
      if (Progress::check_abort() ){
        Rcpp::Rcout << "MCMC aborted" << std::endl;

        return NULL;
      }
      for(int j = 0; j < thin; j++){
        p.increment();

        updateGammaSig(Eta, Lambda, DM2, Phi2, Sigma,
                       Theta, Gamma);

        Phi2 = updatePhistar(Gamma, DM2);
        DM2 = updateDelta(Gamma, Phi2, DM2, a1Gd, a2Gd);

        updateLambdaSig(Eta, Gamma, DM1, Phi1,
                        Sigma, Theta, Lambda);

        Phi1 = updatePhi(Lambda, DM1);
        DM1 = updateDelta(Lambda, Phi1, DM1, a1Ld, a2Ld);

        a1Ld = updatea1(DM1, a1Ld);
        a2Ld = updatea2(DM1, a2Ld);
        a1Gd = updatea1(DM2, a1Gd);
        a2Gd = updatea2(DM2, a2Gd);
        Sigma = updateSigma(Lambda, Gamma, Theta, Eta);
        V = updateVarphi(Theta, splineS, splineT, imputedY);
        updateEta3Sig(Gamma, Lambda, Sigma, Theta, H, X, Beta, Eta);
        Theta = updateThetaSig(Lambda, Gamma, Sigma, Eta,
                               splineS, splineT, imputedY, V);
        H = updateH(Eta, Beta, X);
        updateBeta(Eta, H, E, X, Beta);
        E = updateE(Beta);
        updateMissing(imputedY, missing, Theta, V, splineS, splineT, X.n_rows);

      }
      LambdaF(k).slice(i) = Lambda;
      GammaF(k).slice(i) = Gamma;
      SigmaF(k).slice(i) = Sigma;
      HF(k).slice(i) = H;
      BetaF(k).slice(i) = Beta;
      varphiF(k)(i) = V;
      thetaF(k, i) = Theta;

    }
  }



  Rcpp::List mod = Rcpp::List::create(Rcpp::Named("Lambda", LambdaF),
                                      Rcpp::Named("Gamma", GammaF),
                                      Rcpp::Named("H", HF),
                                      Rcpp::Named("Sigma", SigmaF),
                                      Rcpp::Named("Beta", BetaF),
                                      Rcpp::Named("Varphi", varphiF),
                                      Rcpp::Named("initialY", initialY),
                                      Rcpp::Named("imputedY", imputedY),
                                      Rcpp::Named("Theta", thetaF),
                                      Rcpp::Named("Delta1", DM1F),
                                      Rcpp::Named("Delta2", DM2F)
                                      );


  return(mod);

}
